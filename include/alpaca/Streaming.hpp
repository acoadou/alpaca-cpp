#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_set>
#include <variant>
#include <vector>

#include <ixwebsocket/IXSocketTLSOptions.h>
#include <ixwebsocket/IXWebSocket.h>

// ixwebsocket offers a self-contained TLS-capable client that can run its own
// worker thread.  uWebSockets is geared towards hosting long-lived event loops
// (libuv, ASIO, etc.) which would require consumers of this library to embed
// and drive another reactor.  This client aims to remain plug-and-play for
// applications without an existing event loop, hence the dependency choice.

#include "alpaca/Json.hpp"
#include "alpaca/models/Account.hpp"
#include "alpaca/models/MarketData.hpp"
#include "alpaca/models/Order.hpp"

namespace alpaca::streaming {

/// Enumeration describing the Alpaca websocket feed to connect to.
enum class StreamFeed {
    MarketData,
    Crypto,
    Options,
    Trading
};

/// Distinguishes the semantic type of a streaming payload delivered by Alpaca.
enum class MessageCategory {
    Trade,
    Quote,
    Bar,
    Status,
    Error,
    OrderUpdate,
    AccountUpdate,
    Control,
    Unknown
};

/// Trade update message delivered through market data streams.
struct TradeMessage {
    std::string symbol;
    std::string id;
    std::string exchange;
    double price{0.0};
    std::uint64_t size{0};
    Timestamp timestamp{};
    std::vector<std::string> conditions{};
    std::optional<std::string> tape{};
};

/// Quote update message delivered through market data streams.
struct QuoteMessage {
    std::string symbol;
    std::string ask_exchange;
    double ask_price{0.0};
    std::uint64_t ask_size{0};
    std::string bid_exchange;
    double bid_price{0.0};
    std::uint64_t bid_size{0};
    Timestamp timestamp{};
    std::vector<std::string> conditions{};
    std::optional<std::string> tape{};
};

/// Aggregated bar message delivered through market data streams.
struct BarMessage {
    std::string symbol;
    Timestamp timestamp{};
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    std::uint64_t volume{0};
    std::uint64_t trade_count{0};
    std::optional<double> vwap{};
};

/// Market status or halt/resume notification delivered through market data
/// streams.
struct StatusMessage {
    std::string symbol;
    std::string status_code;
    std::optional<std::string> status_message{};
    std::optional<std::string> reason_code{};
    std::optional<std::string> reason_message{};
    Timestamp timestamp{};
};

/// Order update payload delivered from the trading stream.
struct OrderUpdateMessage {
    std::string event;
    Timestamp event_time{};
    Order order;
};

/// Account update payload delivered from the trading stream.
struct AccountUpdateMessage {
    Account account;
};

/// Error message emitted either by the server or by the websocket
/// implementation.
struct ErrorMessage {
    std::string message;
};

/// Control-plane message delivered by Alpaca (success, subscription, ping,
/// etc.).
struct ControlMessage {
    std::string type;
    Json payload;
};

/// Strongly typed representation of an Alpaca websocket payload.
using StreamMessage = std::variant<TradeMessage, QuoteMessage, BarMessage, StatusMessage, OrderUpdateMessage,
                                   AccountUpdateMessage, ErrorMessage, ControlMessage>;

/// Subscription helper for market data feeds.
struct MarketSubscription {
    std::vector<std::string> trades{};
    std::vector<std::string> quotes{};
    std::vector<std::string> bars{};
    std::vector<std::string> statuses{};
};

/// Callback invoked for every decoded streaming payload.
using MessageHandler = std::function<void(StreamMessage const&, MessageCategory)>;

/// Callback invoked for lifecycle events such as the connection opening or
/// closing.
using LifecycleHandler = std::function<void()>;

/// Callback invoked when the websocket stack reports an error.
using ErrorHandler = std::function<void(std::string const&)>;

/// Configuration describing the exponential backoff strategy for reconnects.
struct ReconnectPolicy {
    std::chrono::milliseconds initial_delay{std::chrono::milliseconds{500}};
    std::chrono::milliseconds max_delay{std::chrono::seconds{30}};
    double multiplier{2.0};
    std::chrono::milliseconds jitter{std::chrono::milliseconds{250}};
};

/// Lightweight websocket client capable of connecting to Alpaca's streaming
/// APIs.
class WebSocketClient {
  public:
    WebSocketClient(std::string url, std::string key, std::string secret, StreamFeed feed = StreamFeed::MarketData);
    ~WebSocketClient();

    WebSocketClient(WebSocketClient const&) = delete;
    WebSocketClient& operator=(WebSocketClient const&) = delete;
    WebSocketClient(WebSocketClient&&) = delete;
    WebSocketClient& operator=(WebSocketClient&&) = delete;

    void connect();
    void disconnect();

    /// Returns true when the websocket connection is currently open.
    bool is_connected() const;

    void subscribe(MarketSubscription const& subscription);
    void unsubscribe(MarketSubscription const& subscription);

    /// Subscribe to trading stream channels (e.g. "trade_updates",
    /// "account_updates").
    void listen(std::vector<std::string> const& streams);

    void send_raw(Json const& message);

    void set_message_handler(MessageHandler handler);
    void set_open_handler(LifecycleHandler handler);
    void set_close_handler(LifecycleHandler handler);
    void set_error_handler(ErrorHandler handler);
    void set_tls_options(ix::SocketTLSOptions options);
    void set_reconnect_policy(ReconnectPolicy policy);
    void set_ping_interval(std::chrono::seconds interval);

  private:
    void authenticate();
    void handle_payload(Json const& payload);
    void handle_control_payload(Json const& payload, std::string const& type);
    void replay_subscriptions();
    void schedule_reconnect();
    void start_socket();
    void start_socket_locked();
    std::chrono::milliseconds compute_backoff_delay(std::size_t attempt);

    std::string url_;
    std::string key_;
    std::string secret_;
    StreamFeed feed_;

    mutable std::mutex connection_mutex_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> should_reconnect_{false};
    bool manual_disconnect_{false};
    std::size_t reconnect_attempt_{0};
    std::thread reconnect_thread_{};

    std::vector<Json> pending_messages_;

    MessageHandler message_handler_{};
    LifecycleHandler open_handler_{};
    LifecycleHandler close_handler_{};
    ErrorHandler error_handler_{};
    ix::SocketTLSOptions tls_options_{};
    bool custom_tls_options_{false};

    ix::WebSocket socket_{};

    std::unordered_set<std::string> subscribed_trades_;
    std::unordered_set<std::string> subscribed_quotes_;
    std::unordered_set<std::string> subscribed_bars_;
    std::unordered_set<std::string> subscribed_statuses_;
    std::unordered_set<std::string> listened_streams_;

    ReconnectPolicy reconnect_policy_{};
    std::mt19937_64 rng_;
    std::chrono::seconds ping_interval_{std::chrono::seconds{30}};
};

} // namespace alpaca::streaming
