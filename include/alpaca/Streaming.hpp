#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
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
#include "alpaca/models/Common.hpp"
#include "alpaca/models/MarketData.hpp"
#include "alpaca/models/News.hpp"
#include "alpaca/models/Order.hpp"

namespace alpaca::streaming {

/// Enumeration describing the Alpaca websocket feed to connect to.
enum class StreamFeed {
    MarketData,
    Crypto,
    Options,
    Trading
};

class BackfillCoordinator;

/// Distinguishes the semantic type of a streaming payload delivered by Alpaca.
enum class MessageCategory {
    Trade,
    Quote,
    Bar,
    UpdatedBar,
    DailyBar,
    OrderBook,
    Luld,
    Auction,
    Greeks,
    Underlying,
    TradeCancel,
    TradeCorrection,
    Imbalance,
    News,
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

/// Near-real-time update to an in-flight minute bar.
struct UpdatedBarMessage {
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

/// End-of-day bar aggregated across the full trading session.
struct DailyBarMessage {
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

/// Represents a single level within an order book side.
struct OrderBookLevel {
    double price{0.0};
    std::uint64_t size{0};
    std::string exchange;
};

/// Depth-of-book update containing bid and ask levels.
struct OrderBookMessage {
    std::string symbol;
    Timestamp timestamp{};
    std::vector<OrderBookLevel> bids{};
    std::vector<OrderBookLevel> asks{};
    std::optional<std::string> tape{};
};

/// Limit-up / limit-down notification for a symbol.
struct LuldMessage {
    std::string symbol;
    Timestamp timestamp{};
    double limit_up{0.0};
    double limit_down{0.0};
    std::optional<std::string> indicator{};
    std::optional<std::string> tape{};
};

/// Opening/closing auction event for a symbol.
struct AuctionMessage {
    std::string symbol;
    Timestamp timestamp{};
    std::optional<std::string> auction_type{};
    std::optional<std::string> condition{};
    std::optional<double> price{};
    std::optional<std::uint64_t> size{};
    std::optional<double> imbalance{};
    std::optional<std::string> imbalance_side{};
    std::optional<std::string> exchange{};
    std::optional<std::string> tape{};
};

/// Option greeks update delivered through the options feed.
struct GreeksMessage {
    std::string symbol;
    Timestamp timestamp{};
    std::optional<double> delta{};
    std::optional<double> gamma{};
    std::optional<double> theta{};
    std::optional<double> vega{};
    std::optional<double> rho{};
    std::optional<double> implied_volatility{};
};

/// Underlying price update delivered through the options feed.
struct UnderlyingMessage {
    std::string symbol;
    std::string underlying_symbol;
    Timestamp timestamp{};
    double price{0.0};
};

/// Notification emitted when a previously reported trade is cancelled.
struct TradeCancelMessage {
    std::string symbol;
    Timestamp timestamp{};
    std::string exchange;
    std::optional<double> price{};
    std::optional<std::uint64_t> size{};
    std::optional<std::string> id{};
    std::optional<std::string> action{};
    std::optional<std::string> tape{};
};

/// Notification emitted when a previously reported trade is corrected.
struct TradeCorrectionMessage {
    std::string symbol;
    Timestamp timestamp{};
    std::string exchange;
    std::optional<std::string> original_id{};
    std::optional<double> original_price{};
    std::optional<std::uint64_t> original_size{};
    std::vector<std::string> original_conditions{};
    std::optional<std::string> corrected_id{};
    std::optional<double> corrected_price{};
    std::optional<std::uint64_t> corrected_size{};
    std::vector<std::string> corrected_conditions{};
    std::optional<std::string> tape{};
};

/// Order imbalance update typically preceding auction events.
struct ImbalanceMessage {
    std::string symbol;
    Timestamp timestamp{};
    std::optional<std::string> exchange{};
    std::optional<std::string> imbalance_side{};
    std::optional<std::uint64_t> imbalance{};
    std::optional<std::uint64_t> paired{};
    std::optional<double> reference_price{};
    std::optional<double> near_price{};
    std::optional<double> far_price{};
    std::optional<double> current_price{};
    std::optional<double> clearing_price{};
    std::optional<std::string> auction_type{};
    std::optional<std::string> tape{};
    Json raw_payload{};
};

/// News headline and article metadata delivered through the websocket feed.
using NewsMessage = NewsArticle;

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
                                   UpdatedBarMessage, DailyBarMessage, OrderBookMessage, LuldMessage, AuctionMessage,
                                   GreeksMessage, UnderlyingMessage, TradeCancelMessage, TradeCorrectionMessage,
                                   ImbalanceMessage, NewsMessage, AccountUpdateMessage, ErrorMessage, ControlMessage>;

/// Subscription helper for market data feeds.
struct MarketSubscription {
    std::vector<std::string> trades{};
    std::vector<std::string> quotes{};
    std::vector<std::string> bars{};
    std::vector<std::string> updated_bars{};
    std::vector<std::string> daily_bars{};
    std::vector<std::string> statuses{};
    std::vector<std::string> orderbooks{};
    std::vector<std::string> lulds{};
    std::vector<std::string> auctions{};
    std::vector<std::string> greeks{};
    std::vector<std::string> underlyings{};
    std::vector<std::string> trade_cancels{};
    std::vector<std::string> trade_corrections{};
    std::vector<std::string> imbalances{};
    std::vector<std::string> news{};
};

/// Callback invoked for every decoded streaming payload.
using MessageHandler = std::function<void(StreamMessage const&, MessageCategory)>;

/// Callback invoked for lifecycle events such as the connection opening or
/// closing.
using LifecycleHandler = std::function<void()>;

/// Callback invoked when the websocket stack reports an error.
using ErrorHandler = std::function<void(std::string const&)>;

/// Policy describing how to detect gaps in streaming sequence identifiers and
/// how to request replays when they are observed.
struct SequenceGapPolicy {
    /// Returns a stable identifier (e.g. symbol or channel) used to track
    /// sequence continuity.
    std::function<std::string(Json const&)> stream_identifier;
    /// Extracts the sequence number from a raw websocket payload. Returning an
    /// empty optional skips tracking for the given payload.
    std::function<std::optional<std::uint64_t>(Json const&)> sequence_extractor;
    /// Invoked when a gap is detected. Receives the stream identifier, the
    /// expected sequence number and the observed sequence number for the
    /// current payload.
    std::function<void(std::string const&, std::uint64_t, std::uint64_t, Json const&)> gap_handler;
    /// Invoked when a gap is detected to request a replay for a specific
    /// sequence range. The range is inclusive of the starting sequence and
    /// inclusive of the ending sequence, enabling callers to bridge the gap.
    std::function<void(std::string const&, std::uint64_t, std::uint64_t, Json const&)> replay_request;
};

/// Configuration describing latency monitoring behaviour for inbound
/// streaming payloads.
struct LatencyMonitor {
    /// Maximum acceptable latency between the event timestamp and the moment it
    /// is processed locally. Values less than or equal to zero disable
    /// monitoring.
    std::chrono::nanoseconds max_latency{std::chrono::nanoseconds::zero()};
    /// Extracts the logical event timestamp from a raw websocket payload.
    std::function<std::optional<Timestamp>(Json const&)> timestamp_extractor;
    /// Produces a stable identifier (e.g. symbol) for reporting purposes.
    std::function<std::string(Json const&)> stream_identifier;
    /// Invoked when latency exceeds the configured threshold. Receives the
    /// stream identifier, the measured latency and the original payload.
    std::function<void(std::string const&, std::chrono::nanoseconds, Json const&)> latency_handler;
};

/// Configuration describing the exponential backoff strategy for reconnects.
struct ReconnectPolicy {
    std::chrono::milliseconds initial_delay{std::chrono::milliseconds{500}};
    std::chrono::milliseconds max_delay{std::chrono::seconds{30}};
    double multiplier{2.0};
    std::chrono::milliseconds jitter{std::chrono::milliseconds{250}};
};

class WebSocketClientHarness;

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

    /// Initiates the connection asynchronously, returning a future that
    /// resolves once the websocket stack has been started.
    std::future<void> connect_async();

    /// Terminates the connection asynchronously.
    std::future<void> disconnect_async();

    /// Returns true when the websocket connection is currently open.
    bool is_connected() const;

    void subscribe(MarketSubscription const& subscription);
    void unsubscribe(MarketSubscription const& subscription);

    /// Subscribes to the provided channels asynchronously.
    std::future<void> subscribe_async(MarketSubscription subscription);

    /// Unsubscribes from the provided channels asynchronously.
    std::future<void> unsubscribe_async(MarketSubscription subscription);

    /// Subscribe to trading stream channels (e.g. "trade_updates",
    /// "account_updates").
    void listen(std::vector<std::string> const& streams);

    /// Asynchronously listen to trading stream channels.
    std::future<void> listen_async(std::vector<std::string> streams);

    void send_raw(Json const& message);

    /// Sends a raw JSON payload asynchronously.
    std::future<void> send_raw_async(Json message);

    void set_message_handler(MessageHandler handler);
    void set_open_handler(LifecycleHandler handler);
    void set_close_handler(LifecycleHandler handler);
    void set_error_handler(ErrorHandler handler);
    void set_tls_options(ix::SocketTLSOptions options);
    void set_reconnect_policy(ReconnectPolicy policy);
    void set_ping_interval(std::chrono::seconds interval);
    void set_heartbeat_timeout(std::chrono::milliseconds timeout);

    /// Sets the maximum number of buffered outbound messages while disconnected.
    /// A value of 0 disables the limit.
    void set_pending_message_limit(std::size_t limit);
    /// Sets the maximum number of buffered inbound messages awaiting
    /// application processing. A value of 0 disables the limit, allowing the
    /// queue to grow without bound.
    void set_incoming_message_limit(std::size_t limit);

    /// Configures sequence gap detection and replay behaviour.
    void set_sequence_gap_policy(SequenceGapPolicy policy);
    /// Disables sequence gap tracking and clears accumulated state.
    void clear_sequence_gap_policy();

    /// Configures latency monitoring for inbound payloads.
    void set_latency_monitor(LatencyMonitor monitor);
    /// Disables latency monitoring.
    void clear_latency_monitor();

    /// Enables automatic REST backfills when sequence gaps are observed.
    void enable_automatic_backfill(std::shared_ptr<BackfillCoordinator> coordinator);

    /// Disables automatic backfills and restores the previous replay handler.
    void disable_automatic_backfill();

  private:
    friend class WebSocketClientHarness;

    void authenticate();
    void handle_payload(Json const& payload);
    void handle_control_payload(Json const& payload, std::string const& type);
    void replay_subscriptions();
    void schedule_reconnect();
    void start_socket();
    void start_socket_locked();
    std::chrono::milliseconds compute_backoff_delay(std::size_t attempt);
    void enqueue_incoming_message(Json payload);
    void dispatcher_loop();
    void start_dispatcher();
    void stop_dispatcher();
    void heartbeat_loop();
    void start_heartbeat();
    void stop_heartbeat();
    void handle_heartbeat_timeout();
    void record_activity();
    void evaluate_sequence_gap(Json const& payload);
    void evaluate_latency(Json const& payload);

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
    std::size_t pending_message_limit_{1024};

    MessageHandler message_handler_{};
    LifecycleHandler open_handler_{};
    LifecycleHandler close_handler_{};
    ErrorHandler error_handler_{};
    ix::SocketTLSOptions tls_options_{};
    bool custom_tls_options_{false};

    ix::WebSocket socket_{};

    std::mutex dispatcher_mutex_;
    std::condition_variable dispatcher_cv_;
    std::deque<Json> inbound_queue_;
    bool dispatcher_running_{false};
    std::thread dispatcher_thread_{};
    std::size_t incoming_message_limit_{4096};

    std::mutex heartbeat_mutex_;
    std::condition_variable heartbeat_cv_;
    bool heartbeat_running_{false};
    std::thread heartbeat_thread_{};
    std::chrono::milliseconds heartbeat_timeout_{std::chrono::milliseconds::zero()};
    std::atomic<std::int64_t> last_message_time_ns_{0};

    std::unordered_set<std::string> subscribed_trades_;
    std::unordered_set<std::string> subscribed_quotes_;
    std::unordered_set<std::string> subscribed_bars_;
    std::unordered_set<std::string> subscribed_updated_bars_;
    std::unordered_set<std::string> subscribed_daily_bars_;
    std::unordered_set<std::string> subscribed_statuses_;
    std::unordered_set<std::string> subscribed_orderbooks_;
    std::unordered_set<std::string> subscribed_lulds_;
    std::unordered_set<std::string> subscribed_auctions_;
    std::unordered_set<std::string> subscribed_greeks_;
    std::unordered_set<std::string> subscribed_underlyings_;
    std::unordered_set<std::string> subscribed_trade_cancels_;
    std::unordered_set<std::string> subscribed_trade_corrections_;
    std::unordered_set<std::string> subscribed_imbalances_;
    std::unordered_set<std::string> subscribed_news_;
    std::unordered_set<std::string> listened_streams_;

    std::mutex sequence_mutex_;
    std::optional<SequenceGapPolicy> sequence_policy_{};
    std::unordered_map<std::string, std::uint64_t> last_sequence_ids_{};
    std::shared_ptr<BackfillCoordinator> backfill_coordinator_{};
    std::function<void(std::string const&, std::uint64_t, std::uint64_t, Json const&)> backfill_passthrough_replay_{};

    std::mutex latency_mutex_;
    std::optional<LatencyMonitor> latency_monitor_{};

    ReconnectPolicy reconnect_policy_{};
    std::mt19937_64 rng_;
    std::chrono::seconds ping_interval_{std::chrono::seconds{30}};
};

} // namespace alpaca::streaming
