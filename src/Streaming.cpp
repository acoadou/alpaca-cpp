#include "alpaca/Streaming.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "alpaca/models/Account.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca::streaming {
namespace {

Timestamp parse_timestamp_field_or_default(Json const& j, char const *key) {
    if (!j.contains(key)) {
        return {};
    }
    auto const& field = j.at(key);
    if (field.is_null()) {
        return {};
    }
    return parse_timestamp(field.get<std::string>());
}

std::vector<std::string> parse_conditions(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return {};
    }
    return j.at(key).get<std::vector<std::string>>();
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
    message.volume = payload.value("v", std::uint64_t{0});
    message.trade_count = payload.value("n", std::uint64_t{0});
    if (payload.contains("vw") && !payload.at("vw").is_null()) {
        message.vwap = payload.at("vw").get<double>();
    }
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
            if (payload.is_array()) {
                for (auto const& entry : payload) {
                    handle_payload(entry);
                }
            } else {
                handle_payload(payload);
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
        for (auto const& symbol : subscription.statuses) {
            if (subscribed_statuses_.insert(symbol).second) {
                diff.statuses.push_back(symbol);
            }
        }
    }

    if (diff.trades.empty() && diff.quotes.empty() && diff.bars.empty() && diff.statuses.empty()) {
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
    if (!diff.statuses.empty()) {
        message["statuses"] = diff.statuses;
    }
    send_raw(message);
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
        for (auto const& symbol : subscription.statuses) {
            if (subscribed_statuses_.erase(symbol) > 0) {
                diff.statuses.push_back(symbol);
            }
        }
    }

    if (diff.trades.empty() && diff.quotes.empty() && diff.bars.empty() && diff.statuses.empty()) {
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
    if (!diff.statuses.empty()) {
        message["statuses"] = diff.statuses;
    }
    send_raw(message);
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

void WebSocketClient::send_raw(Json const& message) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    if (!connected_) {
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
    if (!message_handler_) {
        return;
    }

    if (payload.contains("T")) {
        auto const type = payload.at("T").get<std::string>();
        if (type == "t") {
            message_handler_(build_trade_message(payload), MessageCategory::Trade);
            return;
        }
        if (type == "q") {
            message_handler_(build_quote_message(payload), MessageCategory::Quote);
            return;
        }
        if (type == "b" || type == "d" || type == "o") {
            message_handler_(build_bar_message(payload), MessageCategory::Bar);
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
        snapshot.statuses.assign(subscribed_statuses_.begin(), subscribed_statuses_.end());
        streams.assign(listened_streams_.begin(), listened_streams_.end());
    }

    if (!snapshot.trades.empty() || !snapshot.quotes.empty() || !snapshot.bars.empty() || !snapshot.statuses.empty()) {
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
        if (!snapshot.statuses.empty()) {
            message["statuses"] = snapshot.statuses;
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

} // namespace alpaca::streaming
