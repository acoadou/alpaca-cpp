#include "alpaca/Streaming.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <future>
#include <limits>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "alpaca/models/Account.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca::streaming {
namespace {

std::int64_t steady_now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

Timestamp parse_timestamp_field_or_default(Json const& j, char const* key) {
    if (!j.contains(key)) {
        return {};
    }
    auto const& field = j.at(key);
    if (field.is_null()) {
        return {};
    }
    return parse_timestamp(field.get<std::string>());
}

std::vector<std::string> parse_conditions(Json const& j, char const* key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return {};
    }
    return j.at(key).get<std::vector<std::string>>();
}

template <typename T> std::optional<T> parse_optional(Json const& j, char const* key) {
    if (!j.contains(key)) {
        return std::nullopt;
    }
    auto const& field = j.at(key);
    if (field.is_null()) {
        return std::nullopt;
    }
    return field.get<T>();
}

std::optional<std::uint64_t> parse_optional_uint64(Json const& j, char const* key) {
    if (!j.contains(key)) {
        return std::nullopt;
    }
    auto const& field = j.at(key);
    if (field.is_null()) {
        return std::nullopt;
    }
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
    if (field.is_number_float()) {
        auto value = field.get<double>();
        if (value < 0.0) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(std::llround(value));
    }
    if (field.is_string()) {
        auto text = field.get<std::string>();
        if (text.empty()) {
            return std::nullopt;
        }
        try {
            std::size_t processed = 0;
            auto parsed = std::stoll(text, &processed);
            if (processed != text.size() || parsed < 0) {
                return std::nullopt;
            }
            return static_cast<std::uint64_t>(parsed);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::string> parse_optional_string_like(Json const& j, char const* key) {
    if (!j.contains(key)) {
        return std::nullopt;
    }
    auto const& field = j.at(key);
    if (field.is_null()) {
        return std::nullopt;
    }
    if (field.is_string()) {
        return field.get<std::string>();
    }
    if (field.is_number_unsigned()) {
        return std::to_string(field.get<std::uint64_t>());
    }
    if (field.is_number_integer()) {
        return std::to_string(field.get<std::int64_t>());
    }
    if (field.is_number_float()) {
        return std::to_string(field.get<double>());
    }
    return field.dump();
}

std::vector<OrderBookLevel> parse_order_book_side(Json const& payload, char const* key) {
    if (!payload.contains(key) || !payload.at(key).is_array()) {
        return {};
    }
    std::vector<OrderBookLevel> levels;
    for (auto const& entry : payload.at(key)) {
        OrderBookLevel level{};
        level.price = entry.value("p", 0.0);
        level.size = entry.value("s", std::uint64_t{0});
        level.exchange = entry.value("x", "");
        levels.push_back(std::move(level));
    }
    return levels;
}

StreamMessage build_trade_message(Json const& payload) {
    TradeMessage message{};
    message.symbol = payload.value("S", "");
    message.id = payload.value("i", "");
    message.exchange = payload.value("x", "");
    message.price = payload.value("p", 0.0);
    message.size = payload.value("s", std::uint64_t{0});
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.conditions = parse_conditions(payload, "c");
    if (payload.contains("z") && !payload.at("z").is_null()) {
        message.tape = payload.at("z").get<std::string>();
    }
    return message;
}

StreamMessage build_quote_message(Json const& payload) {
    QuoteMessage message{};
    message.symbol = payload.value("S", "");
    message.ask_exchange = payload.value("ax", "");
    message.ask_price = payload.value("ap", 0.0);
    message.ask_size = payload.value("as", std::uint64_t{0});
    message.bid_exchange = payload.value("bx", "");
    message.bid_price = payload.value("bp", 0.0);
    message.bid_size = payload.value("bs", std::uint64_t{0});
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.conditions = parse_conditions(payload, "c");
    if (payload.contains("z") && !payload.at("z").is_null()) {
        message.tape = payload.at("z").get<std::string>();
    }
    return message;
}

StreamMessage build_bar_message(Json const& payload) {
    BarMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.open = payload.value("o", 0.0);
    message.high = payload.value("h", 0.0);
    message.low = payload.value("l", 0.0);
    message.close = payload.value("c", 0.0);
    if (auto volume = parse_optional_uint64(payload, "v")) {
        message.volume = *volume;
    } else {
        message.volume = payload.value("v", std::uint64_t{0});
    }
    if (auto count = parse_optional_uint64(payload, "n")) {
        message.trade_count = *count;
    } else {
        message.trade_count = payload.value("n", std::uint64_t{0});
    }
    if (auto vwap = parse_optional<double>(payload, "vw")) {
        message.vwap = vwap;
    }
    return message;
}

StreamMessage build_updated_bar_message(Json const& payload) {
    UpdatedBarMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.open = payload.value("o", 0.0);
    message.high = payload.value("h", 0.0);
    message.low = payload.value("l", 0.0);
    message.close = payload.value("c", 0.0);
    if (auto volume = parse_optional_uint64(payload, "v")) {
        message.volume = *volume;
    }
    if (auto count = parse_optional_uint64(payload, "n")) {
        message.trade_count = *count;
    }
    message.vwap = parse_optional<double>(payload, "vw");
    return message;
}

StreamMessage build_daily_bar_message(Json const& payload) {
    DailyBarMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.open = payload.value("o", 0.0);
    message.high = payload.value("h", 0.0);
    message.low = payload.value("l", 0.0);
    message.close = payload.value("c", 0.0);
    if (auto volume = parse_optional_uint64(payload, "v")) {
        message.volume = *volume;
    }
    if (auto count = parse_optional_uint64(payload, "n")) {
        message.trade_count = *count;
    }
    message.vwap = parse_optional<double>(payload, "vw");
    return message;
}

StreamMessage build_status_message(Json const& payload) {
    StatusMessage message{};
    message.symbol = payload.value("S", "");
    message.status_code = payload.value("sc", "");
    if (payload.contains("sm") && !payload.at("sm").is_null()) {
        message.status_message = payload.at("sm").get<std::string>();
    }
    if (payload.contains("rc") && !payload.at("rc").is_null()) {
        message.reason_code = payload.at("rc").get<std::string>();
    }
    if (payload.contains("rm") && !payload.at("rm").is_null()) {
        message.reason_message = payload.at("rm").get<std::string>();
    }
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    return message;
}

StreamMessage build_order_book_message(Json const& payload) {
    OrderBookMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.bids = parse_order_book_side(payload, "b");
    message.asks = parse_order_book_side(payload, "a");
    if (auto tape = parse_optional<std::string>(payload, "z")) {
        message.tape = std::move(tape);
    }
    return message;
}

StreamMessage build_luld_message(Json const& payload) {
    LuldMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.limit_up = payload.value("u", 0.0);
    message.limit_down = payload.value("d", 0.0);
    message.indicator = parse_optional<std::string>(payload, "i");
    message.tape = parse_optional<std::string>(payload, "z");
    return message;
}

StreamMessage build_auction_message(Json const& payload) {
    AuctionMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.auction_type = parse_optional<std::string>(payload, "at");
    if (!message.auction_type) {
        message.auction_type = parse_optional<std::string>(payload, "type");
    }
    message.condition = parse_optional<std::string>(payload, "c");
    if (auto price = parse_optional<double>(payload, "p")) {
        message.price = price;
    } else if (auto open_price = parse_optional<double>(payload, "o")) {
        message.price = open_price;
    }
    if (auto size = parse_optional<std::uint64_t>(payload, "s")) {
        message.size = size;
    }
    message.imbalance = parse_optional<double>(payload, "im");
    if (!message.imbalance) {
        message.imbalance = parse_optional<double>(payload, "i");
    }
    message.imbalance_side = parse_optional<std::string>(payload, "side");
    if (!message.imbalance_side) {
        message.imbalance_side = parse_optional<std::string>(payload, "zv");
    }
    message.exchange = parse_optional<std::string>(payload, "x");
    message.tape = parse_optional<std::string>(payload, "z");
    return message;
}

StreamMessage build_greeks_message(Json const& payload) {
    GreeksMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.delta = parse_optional<double>(payload, "delta");
    message.gamma = parse_optional<double>(payload, "gamma");
    message.theta = parse_optional<double>(payload, "theta");
    message.vega = parse_optional<double>(payload, "vega");
    message.rho = parse_optional<double>(payload, "rho");
    if (!message.rho) {
        message.rho = parse_optional<double>(payload, "r");
    }
    message.implied_volatility = parse_optional<double>(payload, "iv");
    if (!message.implied_volatility) {
        message.implied_volatility = parse_optional<double>(payload, "implied_volatility");
    }
    return message;
}

StreamMessage build_underlying_message(Json const& payload) {
    UnderlyingMessage message{};
    message.symbol = payload.value("S", "");
    message.underlying_symbol = payload.value("uS", payload.value("underlying_symbol", ""));
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.price = payload.value("p", 0.0);
    if (message.price == 0.0 && payload.contains("price") && !payload.at("price").is_null()) {
        message.price = payload.at("price").get<double>();
    }
    return message;
}

StreamMessage build_trade_cancel_message(Json const& payload) {
    TradeCancelMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.exchange = payload.value("x", "");
    message.price = parse_optional<double>(payload, "p");
    message.size = parse_optional_uint64(payload, "s");
    message.id = parse_optional_string_like(payload, "i");
    message.action = parse_optional<std::string>(payload, "a");
    message.tape = parse_optional<std::string>(payload, "z");
    return message;
}

StreamMessage build_trade_correction_message(Json const& payload) {
    TradeCorrectionMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.exchange = payload.value("x", "");
    message.original_id = parse_optional_string_like(payload, "oi");
    message.original_price = parse_optional<double>(payload, "op");
    message.original_size = parse_optional_uint64(payload, "os");
    message.original_conditions = parse_conditions(payload, "oc");
    message.corrected_id = parse_optional_string_like(payload, "ci");
    message.corrected_price = parse_optional<double>(payload, "cp");
    message.corrected_size = parse_optional_uint64(payload, "cs");
    message.corrected_conditions = parse_conditions(payload, "cc");
    message.tape = parse_optional<std::string>(payload, "z");
    return message;
}

StreamMessage build_imbalance_message(Json const& payload) {
    ImbalanceMessage message{};
    message.symbol = payload.value("S", "");
    message.timestamp = parse_timestamp_field_or_default(payload, "t");
    message.exchange = parse_optional<std::string>(payload, "x");
    if (auto imbalance = parse_optional_uint64(payload, "imbalance")) {
        message.imbalance = imbalance;
    } else if (auto imbalance = parse_optional_uint64(payload, "im")) {
        message.imbalance = imbalance;
    } else if (auto imbalance = parse_optional_uint64(payload, "i")) {
        message.imbalance = imbalance;
    }
    if (auto paired = parse_optional_uint64(payload, "paired")) {
        message.paired = paired;
    } else if (auto paired = parse_optional_uint64(payload, "pa")) {
        message.paired = paired;
    }
    if (auto reference_price = parse_optional<double>(payload, "reference_price")) {
        message.reference_price = reference_price;
    } else if (auto reference_price = parse_optional<double>(payload, "rp")) {
        message.reference_price = reference_price;
    }
    if (auto near_price = parse_optional<double>(payload, "near_price")) {
        message.near_price = near_price;
    } else if (auto near_price = parse_optional<double>(payload, "np")) {
        message.near_price = near_price;
    }
    if (auto far_price = parse_optional<double>(payload, "far_price")) {
        message.far_price = far_price;
    } else if (auto far_price = parse_optional<double>(payload, "fp")) {
        message.far_price = far_price;
    }
    if (auto current_price = parse_optional<double>(payload, "current_price")) {
        message.current_price = current_price;
    } else if (auto current_price = parse_optional<double>(payload, "cp")) {
        message.current_price = current_price;
    }
    if (auto clearing_price = parse_optional<double>(payload, "clearing_price")) {
        message.clearing_price = clearing_price;
    } else if (auto clearing_price = parse_optional<double>(payload, "p")) {
        message.clearing_price = clearing_price;
    }
    message.imbalance_side = parse_optional<std::string>(payload, "imbalance_side");
    if (!message.imbalance_side) {
        message.imbalance_side = parse_optional<std::string>(payload, "side");
    }
    if (!message.imbalance_side) {
        message.imbalance_side = parse_optional<std::string>(payload, "zv");
    }
    message.auction_type = parse_optional<std::string>(payload, "auction_type");
    if (!message.auction_type) {
        message.auction_type = parse_optional<std::string>(payload, "at");
    }
    message.tape = parse_optional<std::string>(payload, "z");
    message.raw_payload = payload;
    return message;
}

StreamMessage build_news_message(Json const& payload) {
    NewsArticle article = payload.get<NewsArticle>();
    return article;
}

StreamMessage build_error_message(std::string const& message) {
    return ErrorMessage{message};
}

StreamMessage build_error_message(Json const& payload) {
    if (payload.contains("msg")) {
        return ErrorMessage{payload.at("msg").get<std::string>()};
    }
    return ErrorMessage{payload.dump()};
}

StreamMessage build_control_message(Json const& payload, std::string type) {
    return ControlMessage{std::move(type), payload};
}

StreamMessage build_order_update(Json const& payload) {
    OrderUpdateMessage message{};
    message.event = payload.value("event", "");
    message.event_time = parse_timestamp_field_or_default(payload, "timestamp");
    if (payload.contains("order")) {
        message.order = payload.at("order").get<Order>();
    }
    return message;
}

StreamMessage build_account_update(Json const& payload) {
    AccountUpdateMessage message{};
    message.account = payload.get<Account>();
    return message;
}

bool is_secure_url(std::string const& url) {
    return url.rfind("wss://", 0) == 0;
}

} // namespace

WebSocketClient::WebSocketClient(std::string url, std::string key, std::string secret, StreamFeed feed)
    : url_(std::move(url)), key_(std::move(key)), secret_(std::move(secret)), feed_(feed), rng_(std::random_device{}()) {
    if (is_secure_url(url_)) {
        tls_options_.tls = true;
        tls_options_.caFile = "SYSTEM";
    }

    last_message_time_ns_.store(steady_now_ns());
    start_dispatcher();
    start_heartbeat();

    socket_.disableAutomaticReconnection();
    socket_.setPingInterval(static_cast<uint32_t>(ping_interval_.count()));
    socket_.setOnMessageCallback([this](ix::WebSocketMessagePtr const& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            {
                std::lock_guard<std::mutex> lock(connection_mutex_);
                connected_ = true;
                should_reconnect_ = true;
                manual_disconnect_ = false;
                reconnect_attempt_ = 0;
            }

            record_activity();

            authenticate();
            replay_subscriptions();

            std::vector<Json> pending;
            {
                std::lock_guard<std::mutex> lock(connection_mutex_);
                pending.swap(pending_messages_);
            }
            for (auto const& payload : pending) {
                send_raw(payload);
            }

            if (open_handler_) {
                open_handler_();
            }
            return;
        }

        if (msg->type == ix::WebSocketMessageType::Close) {
            bool should_retry = false;
            {
                std::lock_guard<std::mutex> lock(connection_mutex_);
                connected_ = false;
                should_retry = should_reconnect_ && !manual_disconnect_;
            }
            if (close_handler_) {
                close_handler_();
            }
            if (should_retry) {
                schedule_reconnect();
            }
            return;
        }

        if (msg->type == ix::WebSocketMessageType::Error) {
            bool should_retry = false;
            {
                std::lock_guard<std::mutex> lock(connection_mutex_);
                connected_ = false;
                should_retry = should_reconnect_ && !manual_disconnect_;
            }
            if (error_handler_) {
                error_handler_(msg->errorInfo.reason);
            }
            if (should_retry) {
                schedule_reconnect();
            }
            return;
        }

        if (msg->type != ix::WebSocketMessageType::Message) {
            return;
        }

        try {
            auto payload = Json::parse(msg->str);
            record_activity();
            if (payload.is_array()) {
                for (auto const& entry : payload) {
                    enqueue_incoming_message(entry);
                }
            } else {
                enqueue_incoming_message(payload);
            }
        } catch (std::exception const& ex) {
            if (error_handler_) {
                error_handler_(ex.what());
            }
        }
    });
}

WebSocketClient::~WebSocketClient() {
    disconnect();
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        if (reconnect_thread_.joinable()) {
            thread_to_join = std::move(reconnect_thread_);
        }
    }
    if (thread_to_join.joinable()) {
        thread_to_join.join();
    }

    stop_heartbeat();
    stop_dispatcher();
}

void WebSocketClient::connect() {
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        should_reconnect_ = true;
        manual_disconnect_ = false;
        reconnect_attempt_ = 0;
    }
    start_socket();
}

std::future<void> WebSocketClient::connect_async() {
    return std::async(std::launch::async, [this]() { this->connect(); });
}

void WebSocketClient::disconnect() {
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        should_reconnect_ = false;
        manual_disconnect_ = true;
        if (reconnect_thread_.joinable()) {
            thread_to_join = std::move(reconnect_thread_);
        }
    }
    if (thread_to_join.joinable()) {
        thread_to_join.join();
    }
    socket_.stop();
    connected_ = false;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        pending_messages_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(dispatcher_mutex_);
        inbound_queue_.clear();
    }
}

std::future<void> WebSocketClient::disconnect_async() {
    return std::async(std::launch::async, [this]() { this->disconnect(); });
}

void WebSocketClient::subscribe(MarketSubscription const& subscription) {
    MarketSubscription diff;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        for (auto const& symbol : subscription.trades) {
            if (subscribed_trades_.insert(symbol).second) {
                diff.trades.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.quotes) {
            if (subscribed_quotes_.insert(symbol).second) {
                diff.quotes.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.bars) {
            if (subscribed_bars_.insert(symbol).second) {
                diff.bars.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.updated_bars) {
            if (subscribed_updated_bars_.insert(symbol).second) {
                diff.updated_bars.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.daily_bars) {
            if (subscribed_daily_bars_.insert(symbol).second) {
                diff.daily_bars.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.statuses) {
            if (subscribed_statuses_.insert(symbol).second) {
                diff.statuses.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.orderbooks) {
            if (subscribed_orderbooks_.insert(symbol).second) {
                diff.orderbooks.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.lulds) {
            if (subscribed_lulds_.insert(symbol).second) {
                diff.lulds.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.auctions) {
            if (subscribed_auctions_.insert(symbol).second) {
                diff.auctions.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.greeks) {
            if (subscribed_greeks_.insert(symbol).second) {
                diff.greeks.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.underlyings) {
            if (subscribed_underlyings_.insert(symbol).second) {
                diff.underlyings.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.trade_cancels) {
            if (subscribed_trade_cancels_.insert(symbol).second) {
                diff.trade_cancels.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.trade_corrections) {
            if (subscribed_trade_corrections_.insert(symbol).second) {
                diff.trade_corrections.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.imbalances) {
            if (subscribed_imbalances_.insert(symbol).second) {
                diff.imbalances.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.news) {
            if (subscribed_news_.insert(symbol).second) {
                diff.news.push_back(symbol);
            }
        }
    }

    if (diff.trades.empty() && diff.quotes.empty() && diff.bars.empty() && diff.updated_bars.empty() &&
        diff.daily_bars.empty() && diff.statuses.empty() && diff.orderbooks.empty() && diff.lulds.empty() &&
        diff.auctions.empty() && diff.greeks.empty() && diff.underlyings.empty() && diff.trade_cancels.empty() &&
        diff.trade_corrections.empty() && diff.imbalances.empty() && diff.news.empty()) {
        return;
    }

    Json message;
    message["action"] = "subscribe";
    if (!diff.trades.empty()) {
        message["trades"] = diff.trades;
    }
    if (!diff.quotes.empty()) {
        message["quotes"] = diff.quotes;
    }
    if (!diff.bars.empty()) {
        message["bars"] = diff.bars;
    }
    if (!diff.updated_bars.empty()) {
        message["updatedBars"] = diff.updated_bars;
    }
    if (!diff.daily_bars.empty()) {
        message["dailyBars"] = diff.daily_bars;
    }
    if (!diff.statuses.empty()) {
        message["statuses"] = diff.statuses;
    }
    if (!diff.orderbooks.empty()) {
        message["orderbooks"] = diff.orderbooks;
    }
    if (!diff.lulds.empty()) {
        message["lulds"] = diff.lulds;
    }
    if (!diff.auctions.empty()) {
        message["auctions"] = diff.auctions;
    }
    if (!diff.greeks.empty()) {
        message["greeks"] = diff.greeks;
    }
    if (!diff.underlyings.empty()) {
        message["underlyings"] = diff.underlyings;
    }
    if (!diff.trade_cancels.empty()) {
        message["cancelErrors"] = diff.trade_cancels;
    }
    if (!diff.trade_corrections.empty()) {
        message["corrections"] = diff.trade_corrections;
    }
    if (!diff.imbalances.empty()) {
        message["imbalances"] = diff.imbalances;
    }
    if (!diff.news.empty()) {
        message["news"] = diff.news;
    }
    send_raw(message);
}

std::future<void> WebSocketClient::subscribe_async(MarketSubscription subscription) {
    return std::async(std::launch::async,
                      [this, subscription = std::move(subscription)]() mutable {
        this->subscribe(subscription);
    });
}

void WebSocketClient::unsubscribe(MarketSubscription const& subscription) {
    MarketSubscription diff;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        for (auto const& symbol : subscription.trades) {
            if (subscribed_trades_.erase(symbol) > 0) {
                diff.trades.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.quotes) {
            if (subscribed_quotes_.erase(symbol) > 0) {
                diff.quotes.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.bars) {
            if (subscribed_bars_.erase(symbol) > 0) {
                diff.bars.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.updated_bars) {
            if (subscribed_updated_bars_.erase(symbol) > 0) {
                diff.updated_bars.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.daily_bars) {
            if (subscribed_daily_bars_.erase(symbol) > 0) {
                diff.daily_bars.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.statuses) {
            if (subscribed_statuses_.erase(symbol) > 0) {
                diff.statuses.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.orderbooks) {
            if (subscribed_orderbooks_.erase(symbol) > 0) {
                diff.orderbooks.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.lulds) {
            if (subscribed_lulds_.erase(symbol) > 0) {
                diff.lulds.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.auctions) {
            if (subscribed_auctions_.erase(symbol) > 0) {
                diff.auctions.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.greeks) {
            if (subscribed_greeks_.erase(symbol) > 0) {
                diff.greeks.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.underlyings) {
            if (subscribed_underlyings_.erase(symbol) > 0) {
                diff.underlyings.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.trade_cancels) {
            if (subscribed_trade_cancels_.erase(symbol) > 0) {
                diff.trade_cancels.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.trade_corrections) {
            if (subscribed_trade_corrections_.erase(symbol) > 0) {
                diff.trade_corrections.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.imbalances) {
            if (subscribed_imbalances_.erase(symbol) > 0) {
                diff.imbalances.push_back(symbol);
            }
        }
        for (auto const& symbol : subscription.news) {
            if (subscribed_news_.erase(symbol) > 0) {
                diff.news.push_back(symbol);
            }
        }
    }

    if (diff.trades.empty() && diff.quotes.empty() && diff.bars.empty() && diff.updated_bars.empty() &&
        diff.daily_bars.empty() && diff.statuses.empty() && diff.orderbooks.empty() && diff.lulds.empty() &&
        diff.auctions.empty() && diff.greeks.empty() && diff.underlyings.empty() && diff.trade_cancels.empty() &&
        diff.trade_corrections.empty() && diff.imbalances.empty() && diff.news.empty()) {
        return;
    }

    Json message;
    message["action"] = "unsubscribe";
    if (!diff.trades.empty()) {
        message["trades"] = diff.trades;
    }
    if (!diff.quotes.empty()) {
        message["quotes"] = diff.quotes;
    }
    if (!diff.bars.empty()) {
        message["bars"] = diff.bars;
    }
    if (!diff.updated_bars.empty()) {
        message["updatedBars"] = diff.updated_bars;
    }
    if (!diff.daily_bars.empty()) {
        message["dailyBars"] = diff.daily_bars;
    }
    if (!diff.statuses.empty()) {
        message["statuses"] = diff.statuses;
    }
    if (!diff.orderbooks.empty()) {
        message["orderbooks"] = diff.orderbooks;
    }
    if (!diff.lulds.empty()) {
        message["lulds"] = diff.lulds;
    }
    if (!diff.auctions.empty()) {
        message["auctions"] = diff.auctions;
    }
    if (!diff.greeks.empty()) {
        message["greeks"] = diff.greeks;
    }
    if (!diff.underlyings.empty()) {
        message["underlyings"] = diff.underlyings;
    }
    if (!diff.trade_cancels.empty()) {
        message["cancelErrors"] = diff.trade_cancels;
    }
    if (!diff.trade_corrections.empty()) {
        message["corrections"] = diff.trade_corrections;
    }
    if (!diff.imbalances.empty()) {
        message["imbalances"] = diff.imbalances;
    }
    if (!diff.news.empty()) {
        message["news"] = diff.news;
    }
    send_raw(message);
}

std::future<void> WebSocketClient::unsubscribe_async(MarketSubscription subscription) {
    return std::async(std::launch::async,
                      [this, subscription = std::move(subscription)]() mutable {
        this->unsubscribe(subscription);
    });
}

void WebSocketClient::listen(std::vector<std::string> const& streams) {
    std::vector<std::string> newly_added;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        for (auto const& stream : streams) {
            if (listened_streams_.insert(stream).second) {
                newly_added.push_back(stream);
            }
        }
    }

    if (newly_added.empty()) {
        return;
    }

    Json message;
    message["action"] = "listen";
    message["data"] = Json::object();
    message["data"]["streams"] = newly_added;
    send_raw(message);
}

std::future<void> WebSocketClient::listen_async(std::vector<std::string> streams) {
    return std::async(std::launch::async,
                      [this, streams = std::move(streams)]() mutable { this->listen(streams); });
}

void WebSocketClient::send_raw(Json const& message) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    if (!connected_) {
        if (pending_message_limit_ > 0 && pending_messages_.size() >= pending_message_limit_) {
            if (error_handler_) {
                error_handler_("websocket send queue limit reached; rejecting message");
            }
            throw std::runtime_error("websocket send queue limit reached");
        }
        pending_messages_.push_back(message);
        return;
    }
    auto const payload = message.dump();
    auto const info = socket_.sendText(payload);
    if (!info.success && error_handler_) {
        if (info.compressionError) {
            error_handler_("websocket send failed due to compression error");
        } else {
            error_handler_("websocket send failed");
        }
    }
}

std::future<void> WebSocketClient::send_raw_async(Json message) {
    return std::async(std::launch::async,
                      [this, message = std::move(message)]() mutable { this->send_raw(message); });
}

void WebSocketClient::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void WebSocketClient::set_open_handler(LifecycleHandler handler) {
    open_handler_ = std::move(handler);
}

void WebSocketClient::set_close_handler(LifecycleHandler handler) {
    close_handler_ = std::move(handler);
}

void WebSocketClient::set_error_handler(ErrorHandler handler) {
    error_handler_ = std::move(handler);
}

void WebSocketClient::set_tls_options(ix::SocketTLSOptions options) {
    tls_options_ = std::move(options);
    custom_tls_options_ = true;
}

void WebSocketClient::set_reconnect_policy(ReconnectPolicy policy) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    reconnect_policy_ = std::move(policy);
}

void WebSocketClient::set_ping_interval(std::chrono::seconds interval) {
    if (interval.count() <= 0) {
        throw std::invalid_argument("ping interval must be positive");
    }
    ping_interval_ = interval;
    socket_.setPingInterval(static_cast<uint32_t>(ping_interval_.count()));
}

void WebSocketClient::set_heartbeat_timeout(std::chrono::milliseconds timeout) {
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex_);
        heartbeat_timeout_ = timeout;
    }
    heartbeat_cv_.notify_all();
}

void WebSocketClient::set_pending_message_limit(std::size_t limit) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    pending_message_limit_ = limit;
    if (pending_message_limit_ > 0 && pending_messages_.size() > pending_message_limit_) {
        auto const overflow = pending_messages_.size() - pending_message_limit_;
        pending_messages_.erase(pending_messages_.begin(),
                                pending_messages_.begin() + static_cast<std::ptrdiff_t>(overflow));
        if (error_handler_) {
            error_handler_("websocket send queue trimmed to respect pending message limit");
        }
    }
}

void WebSocketClient::set_incoming_message_limit(std::size_t limit) {
    std::lock_guard<std::mutex> lock(dispatcher_mutex_);
    incoming_message_limit_ = limit;
    if (incoming_message_limit_ > 0 && inbound_queue_.size() > incoming_message_limit_) {
        auto const overflow = inbound_queue_.size() - incoming_message_limit_;
        for (std::size_t i = 0; i < overflow; ++i) {
            inbound_queue_.pop_front();
        }
    }
}

void WebSocketClient::set_sequence_gap_policy(SequenceGapPolicy policy) {
    std::lock_guard<std::mutex> lock(sequence_mutex_);
    sequence_policy_ = std::move(policy);
    last_sequence_ids_.clear();
}

void WebSocketClient::clear_sequence_gap_policy() {
    std::lock_guard<std::mutex> lock(sequence_mutex_);
    sequence_policy_.reset();
    last_sequence_ids_.clear();
}

void WebSocketClient::set_latency_monitor(LatencyMonitor monitor) {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    latency_monitor_ = std::move(monitor);
}

void WebSocketClient::clear_latency_monitor() {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    latency_monitor_.reset();
}

bool WebSocketClient::is_connected() const {
    return connected_.load();
}

void WebSocketClient::authenticate() {
    switch (feed_) {
    case StreamFeed::Trading: {
        Json message;
        message["action"] = "authenticate";
        message["data"] = Json::object();
        message["data"]["key_id"] = key_;
        message["data"]["secret_key"] = secret_;
        send_raw(message);
        break;
    }
    case StreamFeed::MarketData:
    case StreamFeed::Crypto:
    case StreamFeed::Options: {
        Json message;
        message["action"] = "auth";
        message["key"] = key_;
        message["secret"] = secret_;
        send_raw(message);
        break;
    }
    }
}

void WebSocketClient::handle_payload(Json const& payload) {
    evaluate_sequence_gap(payload);
    evaluate_latency(payload);

    if (!message_handler_) {
        return;
    }

    if (payload.contains("T")) {
        auto type = payload.at("T").get<std::string>();
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (type == "t") {
            message_handler_(build_trade_message(payload), MessageCategory::Trade);
            return;
        }
        if (type == "q") {
            message_handler_(build_quote_message(payload), MessageCategory::Quote);
            return;
        }
        if (type == "b") {
            message_handler_(build_bar_message(payload), MessageCategory::Bar);
            return;
        }
        if (type == "u") {
            if (payload.contains("uS") || payload.contains("underlying_symbol")) {
                message_handler_(build_underlying_message(payload), MessageCategory::Underlying);
            } else {
                message_handler_(build_updated_bar_message(payload), MessageCategory::UpdatedBar);
            }
            return;
        }
        if (type == "d") {
            message_handler_(build_daily_bar_message(payload), MessageCategory::DailyBar);
            return;
        }
        if (type == "o") {
            message_handler_(build_order_book_message(payload), MessageCategory::OrderBook);
            return;
        }
        if (type == "l") {
            message_handler_(build_luld_message(payload), MessageCategory::Luld);
            return;
        }
        if (type == "a") {
            message_handler_(build_auction_message(payload), MessageCategory::Auction);
            return;
        }
        if (type == "g") {
            message_handler_(build_greeks_message(payload), MessageCategory::Greeks);
            return;
        }
        if (type == "x") {
            message_handler_(build_trade_cancel_message(payload), MessageCategory::TradeCancel);
            return;
        }
        if (type == "c") {
            message_handler_(build_trade_correction_message(payload), MessageCategory::TradeCorrection);
            return;
        }
        if (type == "i") {
            message_handler_(build_imbalance_message(payload), MessageCategory::Imbalance);
            return;
        }
        if (type == "n") {
            message_handler_(build_news_message(payload), MessageCategory::News);
            return;
        }
        if (type == "s") {
            message_handler_(build_status_message(payload), MessageCategory::Status);
            return;
        }
        if (type == "error") {
            message_handler_(build_error_message(payload), MessageCategory::Error);
            return;
        }
        if (type == "success" || type == "subscription" || type == "cancel" || type == "control" || type == "ping") {
            handle_control_payload(payload, type);
            return;
        }
    }

    if (payload.contains("stream")) {
        auto const stream = payload.at("stream").get<std::string>();
        if (stream == "trade_updates") {
            if (payload.contains("data")) {
                message_handler_(build_order_update(payload.at("data")), MessageCategory::OrderUpdate);
            }
            return;
        }
        if (stream == "account_updates") {
            if (payload.contains("data")) {
                message_handler_(build_account_update(payload.at("data")), MessageCategory::AccountUpdate);
            }
            return;
        }
        handle_control_payload(payload, stream);
        return;
    }

    if (payload.contains("event")) {
        auto const event = payload.at("event").get<std::string>();
        if (event == "trade_updates") {
            if (payload.contains("data")) {
                message_handler_(build_order_update(payload.at("data")), MessageCategory::OrderUpdate);
            }
            return;
        }
        if (event == "account_updates") {
            if (payload.contains("data")) {
                message_handler_(build_account_update(payload.at("data")), MessageCategory::AccountUpdate);
            }
            return;
        }
        if (event == "error") {
            message_handler_(build_error_message(payload), MessageCategory::Error);
            return;
        }
    }

    message_handler_(build_error_message(payload.dump()), MessageCategory::Unknown);
}

void WebSocketClient::handle_control_payload(Json const& payload, std::string const& type) {
    if (type == "ping") {
        Json response;
        response["action"] = "pong";
        send_raw(response);
    }

    if (message_handler_) {
        message_handler_(build_control_message(payload, type), MessageCategory::Control);
    }
}

void WebSocketClient::replay_subscriptions() {
    MarketSubscription snapshot;
    std::vector<std::string> streams;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        snapshot.trades.assign(subscribed_trades_.begin(), subscribed_trades_.end());
        snapshot.quotes.assign(subscribed_quotes_.begin(), subscribed_quotes_.end());
        snapshot.bars.assign(subscribed_bars_.begin(), subscribed_bars_.end());
        snapshot.updated_bars.assign(subscribed_updated_bars_.begin(), subscribed_updated_bars_.end());
        snapshot.daily_bars.assign(subscribed_daily_bars_.begin(), subscribed_daily_bars_.end());
        snapshot.statuses.assign(subscribed_statuses_.begin(), subscribed_statuses_.end());
        snapshot.orderbooks.assign(subscribed_orderbooks_.begin(), subscribed_orderbooks_.end());
        snapshot.lulds.assign(subscribed_lulds_.begin(), subscribed_lulds_.end());
        snapshot.auctions.assign(subscribed_auctions_.begin(), subscribed_auctions_.end());
        snapshot.greeks.assign(subscribed_greeks_.begin(), subscribed_greeks_.end());
        snapshot.underlyings.assign(subscribed_underlyings_.begin(), subscribed_underlyings_.end());
        snapshot.trade_cancels.assign(subscribed_trade_cancels_.begin(), subscribed_trade_cancels_.end());
        snapshot.trade_corrections.assign(subscribed_trade_corrections_.begin(), subscribed_trade_corrections_.end());
        snapshot.imbalances.assign(subscribed_imbalances_.begin(), subscribed_imbalances_.end());
        snapshot.news.assign(subscribed_news_.begin(), subscribed_news_.end());
        streams.assign(listened_streams_.begin(), listened_streams_.end());
    }

    if (!snapshot.trades.empty() || !snapshot.quotes.empty() || !snapshot.bars.empty() ||
        !snapshot.updated_bars.empty() || !snapshot.daily_bars.empty() || !snapshot.statuses.empty() ||
        !snapshot.orderbooks.empty() || !snapshot.lulds.empty() || !snapshot.auctions.empty() ||
        !snapshot.greeks.empty() || !snapshot.underlyings.empty() || !snapshot.trade_cancels.empty() ||
        !snapshot.trade_corrections.empty() || !snapshot.imbalances.empty() || !snapshot.news.empty()) {
        Json message;
        message["action"] = "subscribe";
        if (!snapshot.trades.empty()) {
            message["trades"] = snapshot.trades;
        }
        if (!snapshot.quotes.empty()) {
            message["quotes"] = snapshot.quotes;
        }
        if (!snapshot.bars.empty()) {
            message["bars"] = snapshot.bars;
        }
        if (!snapshot.updated_bars.empty()) {
            message["updatedBars"] = snapshot.updated_bars;
        }
        if (!snapshot.daily_bars.empty()) {
            message["dailyBars"] = snapshot.daily_bars;
        }
        if (!snapshot.statuses.empty()) {
            message["statuses"] = snapshot.statuses;
        }
        if (!snapshot.orderbooks.empty()) {
            message["orderbooks"] = snapshot.orderbooks;
        }
        if (!snapshot.lulds.empty()) {
            message["lulds"] = snapshot.lulds;
        }
        if (!snapshot.auctions.empty()) {
            message["auctions"] = snapshot.auctions;
        }
        if (!snapshot.greeks.empty()) {
            message["greeks"] = snapshot.greeks;
        }
        if (!snapshot.underlyings.empty()) {
            message["underlyings"] = snapshot.underlyings;
        }
        if (!snapshot.trade_cancels.empty()) {
            message["cancelErrors"] = snapshot.trade_cancels;
        }
        if (!snapshot.trade_corrections.empty()) {
            message["corrections"] = snapshot.trade_corrections;
        }
        if (!snapshot.imbalances.empty()) {
            message["imbalances"] = snapshot.imbalances;
        }
        if (!snapshot.news.empty()) {
            message["news"] = snapshot.news;
        }
        send_raw(message);
    }

    if (!streams.empty()) {
        Json message;
        message["action"] = "listen";
        message["data"] = Json::object();
        message["data"]["streams"] = streams;
        send_raw(message);
    }
}

std::chrono::milliseconds WebSocketClient::compute_backoff_delay(std::size_t attempt) {
    if (attempt == 0) {
        attempt = 1;
    }

    double factor = std::pow(reconnect_policy_.multiplier, static_cast<double>(attempt - 1));
    auto base_delay = static_cast<long long>(static_cast<double>(reconnect_policy_.initial_delay.count()) * factor);
    if (base_delay <= 0) {
        base_delay = reconnect_policy_.initial_delay.count();
    }
    std::chrono::milliseconds delay{base_delay};
    if (delay > reconnect_policy_.max_delay) {
        delay = reconnect_policy_.max_delay;
    }

    if (reconnect_policy_.jitter.count() > 0) {
        std::uniform_int_distribution<long long> dist(0, reconnect_policy_.jitter.count());
        auto jitter = std::chrono::milliseconds(dist(rng_));
        if (delay + jitter > reconnect_policy_.max_delay) {
            delay = reconnect_policy_.max_delay;
        } else {
            delay += jitter;
        }
    }

    if (delay.count() <= 0) {
        delay = reconnect_policy_.initial_delay;
    }
    return delay;
}

void WebSocketClient::schedule_reconnect() {
    std::size_t attempt = 0;
    std::thread previous_thread;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        if (!should_reconnect_ || manual_disconnect_) {
            return;
        }
        ++reconnect_attempt_;
        attempt = reconnect_attempt_;
        if (reconnect_thread_.joinable()) {
            previous_thread = std::move(reconnect_thread_);
        }
    }

    if (previous_thread.joinable()) {
        previous_thread.join();
    }

    auto const delay = compute_backoff_delay(attempt);
    std::thread worker([this, delay]() {
        std::this_thread::sleep_for(delay);
        std::lock_guard<std::mutex> lock(connection_mutex_);
        if (!should_reconnect_ || manual_disconnect_) {
            return;
        }
        start_socket_locked();
    });

    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        reconnect_thread_ = std::move(worker);
    }
}

void WebSocketClient::start_socket() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    start_socket_locked();
}

void WebSocketClient::start_socket_locked() {
    if (!custom_tls_options_) {
        if (is_secure_url(url_)) {
            tls_options_.tls = true;
            if (tls_options_.caFile.empty()) {
                tls_options_.caFile = "SYSTEM";
            }
        } else {
            tls_options_.tls = false;
        }
    }

    socket_.setUrl(url_);
    socket_.setTLSOptions(tls_options_);
    socket_.setPingInterval(static_cast<uint32_t>(ping_interval_.count()));
    socket_.start();
}

void WebSocketClient::enqueue_incoming_message(Json payload) {
    std::unique_lock<std::mutex> lock(dispatcher_mutex_);
    if (!dispatcher_running_) {
        lock.unlock();
        handle_payload(payload);
        return;
    }

    if (incoming_message_limit_ > 0 && inbound_queue_.size() >= incoming_message_limit_) {
        inbound_queue_.pop_front();
        if (error_handler_) {
            auto handler = error_handler_;
            lock.unlock();
            handler("Inbound message queue overflow; dropping oldest payload");
            lock.lock();
        }
    }
    inbound_queue_.push_back(std::move(payload));
    lock.unlock();
    dispatcher_cv_.notify_one();
}

void WebSocketClient::dispatcher_loop() {
    std::unique_lock<std::mutex> lock(dispatcher_mutex_);
    while (dispatcher_running_) {
        dispatcher_cv_.wait(lock, [this]() { return !dispatcher_running_ || !inbound_queue_.empty(); });
        if (!dispatcher_running_) {
            break;
        }
        auto payload = std::move(inbound_queue_.front());
        inbound_queue_.pop_front();
        lock.unlock();
        try {
            handle_payload(payload);
        } catch (std::exception const& ex) {
            if (error_handler_) {
                error_handler_(ex.what());
            }
        }
        lock.lock();
    }
}

void WebSocketClient::start_dispatcher() {
    std::lock_guard<std::mutex> lock(dispatcher_mutex_);
    if (dispatcher_running_) {
        return;
    }
    dispatcher_running_ = true;
    dispatcher_thread_ = std::thread([this]() { dispatcher_loop(); });
}

void WebSocketClient::stop_dispatcher() {
    {
        std::lock_guard<std::mutex> lock(dispatcher_mutex_);
        if (!dispatcher_running_) {
            return;
        }
        dispatcher_running_ = false;
    }
    dispatcher_cv_.notify_all();
    if (dispatcher_thread_.joinable()) {
        dispatcher_thread_.join();
    }
}

void WebSocketClient::heartbeat_loop() {
    std::unique_lock<std::mutex> lock(heartbeat_mutex_);
    while (heartbeat_running_) {
        heartbeat_cv_.wait(lock, [this]() { return !heartbeat_running_ || heartbeat_timeout_.count() > 0; });
        if (!heartbeat_running_) {
            break;
        }

        auto const timeout = heartbeat_timeout_;
        if (timeout.count() <= 0) {
            continue;
        }

        auto const last_ns = last_message_time_ns_.load();
        auto const now_ns = steady_now_ns();
        auto const elapsed_ns = now_ns - last_ns;
        auto const timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count();
        if (elapsed_ns >= timeout_ns) {
            lock.unlock();
            handle_heartbeat_timeout();
            last_message_time_ns_.store(now_ns);
            lock.lock();
            continue;
        }

        auto remaining = timeout_ns - elapsed_ns;
        heartbeat_cv_.wait_for(lock, std::chrono::nanoseconds{remaining});
    }
}

void WebSocketClient::start_heartbeat() {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    if (heartbeat_running_) {
        return;
    }
    heartbeat_running_ = true;
    heartbeat_thread_ = std::thread([this]() { heartbeat_loop(); });
}

void WebSocketClient::stop_heartbeat() {
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex_);
        if (!heartbeat_running_) {
            return;
        }
        heartbeat_running_ = false;
    }
    heartbeat_cv_.notify_all();
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

void WebSocketClient::handle_heartbeat_timeout() {
    bool should_retry = false;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        if (!connected_) {
            return;
        }
        connected_ = false;
        should_retry = should_reconnect_ && !manual_disconnect_;
    }

    if (error_handler_) {
        error_handler_("Heartbeat timeout detected");
    }

    socket_.stop();

    if (should_retry) {
        schedule_reconnect();
    }
}

void WebSocketClient::record_activity() {
    last_message_time_ns_.store(steady_now_ns());
    heartbeat_cv_.notify_all();
}

void WebSocketClient::evaluate_sequence_gap(Json const& payload) {
    std::function<void(std::string const&, std::uint64_t, std::uint64_t, Json const&)> gap_handler;
    std::function<void(std::string const&, std::uint64_t, std::uint64_t, Json const&)> replay_handler;
    std::string stream_id;
    std::uint64_t expected = 0;
    std::uint64_t observed = 0;
    bool gap_detected = false;

    {
        std::lock_guard<std::mutex> lock(sequence_mutex_);
        if (!sequence_policy_) {
            return;
        }

        if (sequence_policy_->stream_identifier) {
            stream_id = sequence_policy_->stream_identifier(payload);
        }
        if (stream_id.empty()) {
            return;
        }

        if (!sequence_policy_->sequence_extractor) {
            return;
        }
        auto seq = sequence_policy_->sequence_extractor(payload);
        if (!seq.has_value()) {
            return;
        }

        auto const current = *seq;
        auto const it = last_sequence_ids_.find(stream_id);
        if (it == last_sequence_ids_.end()) {
            last_sequence_ids_.emplace(stream_id, current);
            return;
        }

        auto const previous = it->second;
        expected = previous + 1;
        observed = current;
        if (observed > expected) {
            gap_detected = true;
        }
        if (observed > previous) {
            it->second = observed;
        }

        gap_handler = sequence_policy_->gap_handler;
        replay_handler = sequence_policy_->replay_request;
    }

    if (gap_detected) {
        if (gap_handler) {
            gap_handler(stream_id, expected, observed, payload);
        }
        if (replay_handler && observed > expected) {
            replay_handler(stream_id, expected, observed - 1, payload);
        }
    }
}

void WebSocketClient::evaluate_latency(Json const& payload) {
    std::optional<LatencyMonitor> monitor;
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        if (!latency_monitor_) {
            return;
        }
        monitor = latency_monitor_;
    }

    if (!monitor->latency_handler || monitor->max_latency <= std::chrono::nanoseconds::zero()) {
        return;
    }

    if (!monitor->timestamp_extractor) {
        return;
    }
    auto event_ts = monitor->timestamp_extractor(payload);
    if (!event_ts.has_value()) {
        return;
    }

    auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
    auto latency = now - *event_ts;
    if (latency <= monitor->max_latency) {
        return;
    }

    std::string stream_id;
    if (monitor->stream_identifier) {
        stream_id = monitor->stream_identifier(payload);
    }

    monitor->latency_handler(stream_id, latency, payload);
}

} // namespace alpaca::streaming
