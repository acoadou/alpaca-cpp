#include "alpaca/BackfillCoordinator.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <utility>

#include "alpaca/MarketDataClient.hpp"
#include "alpaca/Streaming.hpp"

namespace alpaca::streaming {

namespace {
std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string extract_symbol_from_stream_id(std::string const& stream_id) {
    auto const pos = stream_id.find('|');
    if (pos == std::string::npos) {
        return stream_id;
    }
    return stream_id.substr(pos + 1);
}

std::string make_state_key(std::string const& stream_id, std::string const& suffix) {
    std::string key = extract_symbol_from_stream_id(stream_id);
    key.push_back('|');
    key.append(suffix);
    return key;
}

std::optional<std::uint64_t> parse_sequence_value(Json const& payload, char const* key) {
    if (!payload.contains(key) || payload.at(key).is_null()) {
        return std::nullopt;
    }
    auto const& field = payload.at(key);
    if (field.is_number_unsigned()) {
        return field.get<std::uint64_t>();
    }
    if (field.is_number_integer()) {
        auto value = field.get<std::int64_t>();
        if (value < 0) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(value);
    }
    if (field.is_string()) {
        auto const text = field.get<std::string>();
        if (text.empty()) {
            return std::nullopt;
        }
        try {
            return static_cast<std::uint64_t>(std::stoull(text));
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::uint64_t> extract_sequence(Json const& payload) {
    if (auto seq = parse_sequence_value(payload, "i")) {
        return seq;
    }
    if (auto seq = parse_sequence_value(payload, "sequence")) {
        return seq;
    }
    if (auto seq = parse_sequence_value(payload, "seq")) {
        return seq;
    }
    return std::nullopt;
}

std::optional<Timestamp> extract_timestamp_from_payload(Json const& payload) {
    if (auto ts = parse_timestamp_field(payload, "t")) {
        return ts;
    }
    if (payload.contains("timestamp") && !payload.at("timestamp").is_null()) {
        auto const& field = payload.at("timestamp");
        if (field.is_string()) {
            return parse_timestamp(field.get<std::string>());
        }
    }
    return std::nullopt;
}

void append_optional_limit(std::optional<int>& target, int limit) {
    if (limit > 0) {
        target = limit;
    }
}

} // namespace

BackfillCoordinator::BackfillCoordinator(MarketDataClient& market_data_client, StreamFeed feed)
  : BackfillCoordinator(market_data_client, feed, Options{}) {
}

BackfillCoordinator::BackfillCoordinator(MarketDataClient& market_data_client, StreamFeed feed, Options options)
  : market_data_client_(&market_data_client), feed_(feed), options_(std::move(options)) {
    if (!options_.bar_timeframe.has_value() && (feed == StreamFeed::MarketData || feed == StreamFeed::Options)) {
        options_.bar_timeframe = TimeFrame::minute();
    }
}

void BackfillCoordinator::set_trade_replay_handler(TradeReplayHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    trade_handler_ = std::move(handler);
}

void BackfillCoordinator::set_bar_replay_handler(BarReplayHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    bar_handler_ = std::move(handler);
}

void BackfillCoordinator::record_payload(std::string const& stream_id, Json const& payload) {
    auto const timestamp = extract_timestamp(payload);
    if (!timestamp.has_value()) {
        return;
    }

    auto const payload_kind = classify_payload(payload);
    if (!payload_kind.has_value()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto const kind_suffix = *payload_kind == PayloadKind::Trade ? std::string{"trade"} : std::string{"bar"};
    auto& state = states_[make_state_key(stream_id, kind_suffix)];
    state.previous_timestamp = state.last_timestamp;
    state.last_timestamp = *timestamp;

    if (auto sequence = extract_sequence(payload)) {
        if (state.last_requested_range && *sequence >= state.last_requested_range->second) {
            state.last_requested_range.reset();
        }
    }
}

void BackfillCoordinator::request_backfill(std::string const& stream_id, std::uint64_t from_sequence,
                                           std::uint64_t to_sequence, Json const& payload) {
    if (!market_data_client_ || from_sequence > to_sequence) {
        return;
    }

    auto const payload_kind = classify_payload(payload);
    if (!payload_kind.has_value()) {
        return;
    }

    auto const observed_timestamp = extract_timestamp(payload);
    if (!observed_timestamp.has_value()) {
        return;
    }

    auto const kind_suffix = *payload_kind == PayloadKind::Trade ? std::string{"trade"} : std::string{"bar"};
    auto const state_key = make_state_key(stream_id, kind_suffix);
    auto const symbol = extract_symbol_from_stream_id(stream_id);

    StreamState state_copy{};
    TradeReplayHandler trade_handler_copy{};
    BarReplayHandler bar_handler_copy{};
    bool skip_request = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& state = states_[state_key];
        if (state.last_requested_range.has_value() && from_sequence >= state.last_requested_range->first &&
            to_sequence <= state.last_requested_range->second) {
            skip_request = true;
        } else {
            auto range = std::make_pair(from_sequence, to_sequence);
            if (state.last_requested_range.has_value()) {
                range.first = std::min(range.first, state.last_requested_range->first);
                range.second = std::max(range.second, state.last_requested_range->second);
            }
            state.last_requested_range = range;
        }
        state_copy = state;
        trade_handler_copy = trade_handler_;
        bar_handler_copy = bar_handler_;
    }

    if (skip_request) {
        return;
    }

    std::optional<Timestamp> start_timestamp = state_copy.previous_timestamp;
    if (!start_timestamp.has_value() && state_copy.last_timestamp.has_value()) {
        start_timestamp = state_copy.last_timestamp;
    }
    if (!start_timestamp.has_value()) {
        start_timestamp = observed_timestamp;
    }

    Timestamp start = *start_timestamp;
    Timestamp end = *observed_timestamp;
    if (start > end) {
        std::swap(start, end);
    }

    auto const span = to_sequence - from_sequence + 1;
    auto const capped = std::min<std::uint64_t>(span, static_cast<std::uint64_t>(std::numeric_limits<int>::max()));
    auto const limit = static_cast<int>(capped);

    switch (*payload_kind) {
    case PayloadKind::Trade:
        if (options_.request_trades) {
            replay_trades(symbol, start, end, limit, trade_handler_copy);
        }
        break;
    case PayloadKind::Bar:
        if (options_.request_bars) {
            replay_bars(symbol, start, end, limit, bar_handler_copy);
        }
        break;
    }
}

std::optional<Timestamp> BackfillCoordinator::extract_timestamp(Json const& payload) const {
    return extract_timestamp_from_payload(payload);
}

std::optional<BackfillCoordinator::PayloadKind> BackfillCoordinator::classify_payload(Json const& payload) const {
    if (payload.contains("T") && payload.at("T").is_string()) {
        auto type = to_lower_copy(payload.at("T").get<std::string>());
        if (type == "t") {
            return PayloadKind::Trade;
        }
        if (type == "b" || type == "u") {
            return PayloadKind::Bar;
        }
    }
    if (payload.contains("ev") && payload.at("ev").is_string()) {
        auto type = to_lower_copy(payload.at("ev").get<std::string>());
        if (type == "trade") {
            return PayloadKind::Trade;
        }
        if (type == "bar") {
            return PayloadKind::Bar;
        }
    }
    return std::nullopt;
}

void BackfillCoordinator::replay_trades(std::string const& symbol, Timestamp start, Timestamp end, int limit,
                                        TradeReplayHandler const& handler) {
    if (!market_data_client_) {
        return;
    }

    switch (feed_) {
    case StreamFeed::MarketData: {
        MultiStockTradesRequest request;
        request.symbols = {symbol};
        request.start = start;
        request.end = end;
        request.sort = SortDirection::ASC;
        append_optional_limit(request.limit, limit);
        auto const response = market_data_client_->get_stock_trades(request);
        if (handler) {
            auto it = response.trades.find(symbol);
            if (it != response.trades.end()) {
                handler(symbol, it->second);
            } else {
                static std::vector<StockTrade> const empty_trades;
                handler(symbol, empty_trades);
            }
        }
        break;
    }
    case StreamFeed::Options: {
        MultiOptionTradesRequest request;
        request.symbols = {symbol};
        request.start = start;
        request.end = end;
        request.sort = SortDirection::ASC;
        append_optional_limit(request.limit, limit);
        auto const response = market_data_client_->get_option_trades(request);
        if (handler) {
            auto it = response.trades.find(symbol);
            if (it != response.trades.end()) {
                handler(symbol, it->second);
            } else {
                static std::vector<OptionTrade> const empty_trades;
                handler(symbol, empty_trades);
            }
        }
        break;
    }
    case StreamFeed::Crypto: {
        MultiCryptoTradesRequest request;
        request.symbols = {symbol};
        request.start = start;
        request.end = end;
        request.sort = SortDirection::ASC;
        append_optional_limit(request.limit, limit);
        request.feed = options_.crypto_feed;
        auto const response = market_data_client_->get_crypto_trades(request);
        if (handler) {
            auto it = response.trades.find(symbol);
            if (it != response.trades.end()) {
                handler(symbol, it->second);
            } else {
                static std::vector<CryptoTrade> const empty_trades;
                handler(symbol, empty_trades);
            }
        }
        break;
    }
    case StreamFeed::Trading:
        break;
    }
}

void BackfillCoordinator::replay_bars(std::string const& symbol, Timestamp start, Timestamp end, int limit,
                                      BarReplayHandler const& handler) {
    if (!market_data_client_) {
        return;
    }

    switch (feed_) {
    case StreamFeed::MarketData: {
        MultiStockBarsRequest request;
        request.symbols = {symbol};
        request.start = start;
        request.end = end;
        request.sort = SortDirection::ASC;
        append_optional_limit(request.limit, limit);
        request.timeframe = options_.bar_timeframe;
        auto const response = market_data_client_->get_stock_aggregates(request);
        if (handler) {
            auto it = response.bars.find(symbol);
            if (it != response.bars.end()) {
                handler(symbol, it->second);
            } else {
                static std::vector<StockBar> const empty_bars;
                handler(symbol, empty_bars);
            }
        }
        break;
    }
    case StreamFeed::Options: {
        MultiOptionBarsRequest request;
        request.symbols = {symbol};
        request.start = start;
        request.end = end;
        request.sort = SortDirection::ASC;
        append_optional_limit(request.limit, limit);
        request.timeframe = options_.bar_timeframe;
        auto const response = market_data_client_->get_option_aggregates(request);
        if (handler) {
            auto it = response.bars.find(symbol);
            if (it != response.bars.end()) {
                handler(symbol, it->second);
            } else {
                static std::vector<OptionBar> const empty_bars;
                handler(symbol, empty_bars);
            }
        }
        break;
    }
    case StreamFeed::Crypto: {
        MultiCryptoBarsRequest request;
        request.symbols = {symbol};
        request.start = start;
        request.end = end;
        request.sort = SortDirection::ASC;
        append_optional_limit(request.limit, limit);
        request.timeframe = options_.bar_timeframe;
        request.feed = options_.crypto_feed;
        auto const response = market_data_client_->get_crypto_aggregates(request);
        if (handler) {
            auto it = response.bars.find(symbol);
            if (it != response.bars.end()) {
                handler(symbol, it->second);
            } else {
                static std::vector<CryptoBar> const empty_bars;
                handler(symbol, empty_bars);
            }
        }
        break;
    }
    case StreamFeed::Trading:
        break;
    }
}

} // namespace alpaca::streaming
