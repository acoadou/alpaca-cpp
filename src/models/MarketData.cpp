#include "alpaca/models/MarketData.hpp"

#include <stdexcept>

namespace alpaca {

namespace {
template <typename T> std::optional<T> optional_field(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    return j.at(key).get<T>();
}

template <typename Item>
void parse_symbol_collection(Json const& j, char const *key, std::map<std::string, std::vector<Item>>& out) {
    out.clear();
    if (!j.contains(key) || !j.at(key).is_object()) {
        return;
    }
    for (auto const& [symbol, value] : j.at(key).items()) {
        out.emplace(symbol, value.template get<std::vector<Item>>());
    }
}
} // namespace

void from_json(Json const& j, StockTrade& trade) {
    j.at("i").get_to(trade.id);
    j.at("x").get_to(trade.exchange);
    j.at("p").get_to(trade.price);
    j.at("s").get_to(trade.size);
    trade.timestamp = parse_timestamp(j.at("t").get<std::string>());
    if (j.contains("c")) {
        j.at("c").get_to(trade.conditions);
    } else {
        trade.conditions.clear();
    }
    trade.tape = optional_field<std::string>(j, "z");
}

void from_json(Json const& j, StockQuote& quote) {
    j.at("ax").get_to(quote.ask_exchange);
    j.at("ap").get_to(quote.ask_price);
    j.at("as").get_to(quote.ask_size);
    j.at("bx").get_to(quote.bid_exchange);
    j.at("bp").get_to(quote.bid_price);
    j.at("bs").get_to(quote.bid_size);
    quote.timestamp = parse_timestamp(j.at("t").get<std::string>());
    if (j.contains("c")) {
        j.at("c").get_to(quote.conditions);
    } else {
        quote.conditions.clear();
    }
    quote.tape = optional_field<std::string>(j, "z");
}

void from_json(Json const& j, StockBar& bar) {
    bar.timestamp = parse_timestamp(j.at("t").get<std::string>());
    j.at("o").get_to(bar.open);
    j.at("h").get_to(bar.high);
    j.at("l").get_to(bar.low);
    j.at("c").get_to(bar.close);
    j.at("v").get_to(bar.volume);
    if (j.contains("n")) {
        j.at("n").get_to(bar.trade_count);
    } else {
        bar.trade_count = 0;
    }
    bar.vwap = optional_field<double>(j, "vw");
}

void from_json(Json const& j, LatestStockTrade& response) {
    j.at("symbol").get_to(response.symbol);
    j.at("trade").get_to(response.trade);
}

void from_json(Json const& j, LatestStockQuote& response) {
    j.at("symbol").get_to(response.symbol);
    j.at("quote").get_to(response.quote);
}

void from_json(Json const& j, StockBars& response) {
    j.at("symbol").get_to(response.symbol);
    if (j.contains("bars")) {
        j.at("bars").get_to(response.bars);
    } else {
        response.bars.clear();
    }
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, StockSnapshot& snapshot) {
    j.at("symbol").get_to(snapshot.symbol);
    snapshot.latest_trade = optional_field<StockTrade>(j, "latestTrade");
    snapshot.latest_quote = optional_field<StockQuote>(j, "latestQuote");
    snapshot.minute_bar = optional_field<StockBar>(j, "minuteBar");
    snapshot.daily_bar = optional_field<StockBar>(j, "dailyBar");
    snapshot.previous_daily_bar = optional_field<StockBar>(j, "prevDailyBar");
}

void from_json(Json const& j, MultiStockBars& response) {
    parse_symbol_collection(j, "bars", response.bars);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiStockQuotes& response) {
    parse_symbol_collection(j, "quotes", response.quotes);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiStockTrades& response) {
    parse_symbol_collection(j, "trades", response.trades);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiOptionBars& response) {
    parse_symbol_collection(j, "bars", response.bars);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiOptionQuotes& response) {
    parse_symbol_collection(j, "quotes", response.quotes);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiOptionTrades& response) {
    parse_symbol_collection(j, "trades", response.trades);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiCryptoBars& response) {
    parse_symbol_collection(j, "bars", response.bars);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiCryptoQuotes& response) {
    parse_symbol_collection(j, "quotes", response.quotes);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(Json const& j, MultiCryptoTrades& response) {
    parse_symbol_collection(j, "trades", response.trades);
    response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

namespace {
void append_timestamp(QueryParams& params, std::string const& key, std::optional<Timestamp> const& value) {
    if (value.has_value()) {
        params.emplace_back(key, format_timestamp(*value));
    }
}

void append_csv(QueryParams& params, std::string const& key, std::vector<std::string> const& values) {
    if (!values.empty()) {
        params.emplace_back(key, join_csv(values));
    }
}

void append_limit(QueryParams& params, std::optional<int> const& limit) {
    if (!limit.has_value()) {
        return;
    }
    if (*limit <= 0) {
        throw std::invalid_argument("limit must be positive");
    }
    params.emplace_back("limit", std::to_string(*limit));
}

void append_sort(QueryParams& params, std::optional<SortDirection> const& sort) {
    if (sort.has_value()) {
        params.emplace_back("sort", to_string(*sort));
    }
}
} // namespace

QueryParams StockBarsRequest::to_query_params() const {
    QueryParams params;
    if (!timeframe.empty()) {
        params.emplace_back("timeframe", timeframe);
    }
    append_timestamp(params, "start", start);
    append_timestamp(params, "end", end);
    if (limit.has_value()) {
        params.emplace_back("limit", std::to_string(*limit));
    }
    if (adjustment.has_value()) {
        params.emplace_back("adjustment", *adjustment);
    }
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    append_timestamp(params, "asof", asof);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    return params;
}

QueryParams NewsRequest::to_query_params() const {
    QueryParams params;
    append_csv(params, "symbols", symbols);
    append_timestamp(params, "start", start);
    append_timestamp(params, "end", end);
    if (start.has_value() && end.has_value() && *start > *end) {
        throw std::invalid_argument("news request start must be <= end");
    }
    append_limit(params, limit);
    append_sort(params, sort);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    if (include_content) {
        params.emplace_back("include_content", "true");
    }
    return params;
}

QueryParams CorporateActionAnnouncementsRequest::to_query_params() const {
    QueryParams params;
    append_csv(params, "symbols", symbols);
    append_csv(params, "ca_types", corporate_action_types);
    append_timestamp(params, "since", since);
    append_timestamp(params, "until", until);
    if (since.has_value() && until.has_value() && *since > *until) {
        throw std::invalid_argument("announcements since must be <= until");
    }
    append_limit(params, limit);
    append_sort(params, sort);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    return params;
}

QueryParams CorporateActionEventsRequest::to_query_params() const {
    QueryParams params;
    append_csv(params, "symbols", symbols);
    append_csv(params, "ca_types", corporate_action_types);
    append_timestamp(params, "since", since);
    append_timestamp(params, "until", until);
    if (since.has_value() && until.has_value() && *since > *until) {
        throw std::invalid_argument("events since must be <= until");
    }
    append_limit(params, limit);
    append_sort(params, sort);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    return params;
}

namespace {
void validate_symbols(std::vector<std::string> const& symbols) {
    if (symbols.empty()) {
        throw std::invalid_argument("at least one symbol must be provided");
    }
}
} // namespace

QueryParams MultiBarsRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (timeframe.has_value() && timeframe->empty()) {
        throw std::invalid_argument("timeframe cannot be empty when provided");
    }
    if (timeframe.has_value()) {
        params.emplace_back("timeframe", *timeframe);
    }
    append_timestamp(params, "start", start);
    append_timestamp(params, "end", end);
    if (start.has_value() && end.has_value() && *start > *end) {
        throw std::invalid_argument("bars start must be <= end");
    }
    append_limit(params, limit);
    append_sort(params, sort);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    if (adjustment.has_value()) {
        params.emplace_back("adjustment", *adjustment);
    }
    append_timestamp(params, "asof", asof);
    return params;
}

QueryParams MultiQuotesRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    append_timestamp(params, "start", start);
    append_timestamp(params, "end", end);
    if (start.has_value() && end.has_value() && *start > *end) {
        throw std::invalid_argument("quotes start must be <= end");
    }
    append_limit(params, limit);
    append_sort(params, sort);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams MultiTradesRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    append_timestamp(params, "start", start);
    append_timestamp(params, "end", end);
    if (start.has_value() && end.has_value() && *start > *end) {
        throw std::invalid_argument("trades start must be <= end");
    }
    append_limit(params, limit);
    append_sort(params, sort);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

} // namespace alpaca
