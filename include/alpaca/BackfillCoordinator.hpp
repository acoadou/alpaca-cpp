#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/models/Common.hpp"
#include "alpaca/models/MarketData.hpp"

namespace alpaca {
class MarketDataClient;
} // namespace alpaca

namespace alpaca::streaming {

enum class StreamFeed;

/// Coordinates REST backfill requests when sequence gaps are detected on the
/// streaming connection.
class BackfillCoordinator {
  public:
    struct Options {
        /// Timeframe used when querying bar replays. Defaults to one minute for
        /// equities and options feeds.
        std::optional<alpaca::TimeFrame> bar_timeframe{};
        /// Feed identifier passed to crypto REST endpoints when replaying
        /// market data.
        std::optional<std::string> crypto_feed{};
        /// Enables replaying missing trade data.
        bool request_trades{true};
        /// Enables replaying missing bar data.
        bool request_bars{true};
    };

    using TradeReplayHandler = std::function<void(std::string const&, std::vector<alpaca::StockTrade> const&)>;
    using BarReplayHandler = std::function<void(std::string const&, std::vector<alpaca::StockBar> const&)>;

    BackfillCoordinator(MarketDataClient& market_data_client, StreamFeed feed);
    BackfillCoordinator(MarketDataClient& market_data_client, StreamFeed feed, Options options);

    void set_trade_replay_handler(TradeReplayHandler handler);
    void set_bar_replay_handler(BarReplayHandler handler);

    /// Records the latest timestamp observed for a stream identifier so the
    /// coordinator can derive replay windows for future sequence gaps.
    void record_payload(std::string const& stream_id, Json const& payload);

    /// Invoked when a sequence gap is detected. Dispatches REST calls to fetch
    /// missing records for the provided stream identifier.
    void request_backfill(std::string const& stream_id, std::uint64_t from_sequence, std::uint64_t to_sequence,
                          Json const& payload);

  private:
    enum class PayloadKind {
        Trade,
        Bar
    };

    struct StreamState {
        std::optional<Timestamp> previous_timestamp{};
        std::optional<Timestamp> last_timestamp{};
        std::optional<std::pair<std::uint64_t, std::uint64_t>> last_requested_range{};
    };

    [[nodiscard]] std::optional<Timestamp> extract_timestamp(Json const& payload) const;
    [[nodiscard]] std::optional<PayloadKind> classify_payload(Json const& payload) const;

    void replay_trades(std::string const& stream_id, Timestamp start, Timestamp end, int limit,
                       TradeReplayHandler const& handler);
    void replay_bars(std::string const& stream_id, Timestamp start, Timestamp end, int limit,
                     BarReplayHandler const& handler);

    MarketDataClient* market_data_client_;
    StreamFeed feed_;
    Options options_;

    std::mutex mutex_;
    std::unordered_map<std::string, StreamState> states_;
    TradeReplayHandler trade_handler_{};
    BarReplayHandler bar_handler_{};
};

} // namespace alpaca::streaming
