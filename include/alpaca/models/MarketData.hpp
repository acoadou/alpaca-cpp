#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"
#include "alpaca/models/Option.hpp"

namespace alpaca {

/// Represents an individual trade returned by Alpaca Market Data.
struct StockTrade {
    std::string id;
    std::string exchange;
    double price{0.0};
    std::uint64_t size{0};
    Timestamp timestamp{};
    std::vector<std::string> conditions{};
    std::optional<std::string> tape{};
};

void from_json(Json const& j, StockTrade& trade);

/// Represents an individual quote returned by Alpaca Market Data.
struct StockQuote {
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

void from_json(Json const& j, StockQuote& quote);

/// Represents an OHLCV bar from Alpaca Market Data.
struct StockBar {
    Timestamp timestamp{};
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    std::uint64_t volume{0};
    std::uint64_t trade_count{0};
    std::optional<double> vwap{};
};

void from_json(Json const& j, StockBar& bar);

/// Response wrapper containing the latest trade for a symbol.
struct LatestStockTrade {
    std::string symbol;
    StockTrade trade;
};

void from_json(Json const& j, LatestStockTrade& response);

/// Response wrapper containing the latest quote for a symbol.
struct LatestStockQuote {
    std::string symbol;
    StockQuote quote;
};

void from_json(Json const& j, LatestStockQuote& response);

/// Response wrapper containing a collection of bars for a symbol.
struct StockBars {
    std::string symbol;
    std::vector<StockBar> bars;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, StockBars& response);

/// Request parameters for the per-symbol bars endpoint.
struct StockBarsRequest {
    std::string timeframe;
    std::optional<Timestamp> start{};
    std::optional<Timestamp> end{};
    std::optional<int> limit{};
    std::optional<std::string> adjustment{};
    std::optional<std::string> feed{};
    std::optional<Timestamp> asof{};
    std::optional<std::string> page_token{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Snapshot structure exposing the latest market data aggregates for a symbol.
struct StockSnapshot {
    std::string symbol;
    std::optional<StockTrade> latest_trade{};
    std::optional<StockQuote> latest_quote{};
    std::optional<StockBar> minute_bar{};
    std::optional<StockBar> daily_bar{};
    std::optional<StockBar> previous_daily_bar{};
};

void from_json(Json const& j, StockSnapshot& snapshot);

/// Response wrapper containing multi-symbol stock snapshots keyed by symbol.
struct MultiStockSnapshots {
    std::map<std::string, StockSnapshot> snapshots;
};

void from_json(Json const& j, MultiStockSnapshots& response);

/// Response wrapper containing multi-symbol stock aggregates.
struct MultiStockBars {
    std::map<std::string, std::vector<StockBar>> bars;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiStockBars& response);

/// Response wrapper containing multi-symbol stock quotes.
struct MultiStockQuotes {
    std::map<std::string, std::vector<StockQuote>> quotes;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiStockQuotes& response);

/// Response wrapper containing multi-symbol stock trades.
struct MultiStockTrades {
    std::map<std::string, std::vector<StockTrade>> trades;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiStockTrades& response);

/// Aliases for option and crypto market data types which share the same schema
/// as equities.
using OptionBar = StockBar;
using OptionQuote = StockQuote;
using OptionTrade = StockTrade;
using CryptoBar = StockBar;
using CryptoQuote = StockQuote;
using CryptoTrade = StockTrade;

/// Response wrapper containing multi-symbol option aggregates.
struct MultiOptionBars {
    std::map<std::string, std::vector<OptionBar>> bars;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiOptionBars& response);

/// Response wrapper containing multi-symbol option quotes.
struct MultiOptionQuotes {
    std::map<std::string, std::vector<OptionQuote>> quotes;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiOptionQuotes& response);

/// Response wrapper containing multi-symbol option trades.
struct MultiOptionTrades {
    std::map<std::string, std::vector<OptionTrade>> trades;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiOptionTrades& response);

/// Collection of intraday metrics exposed via the options snapshot endpoint.
struct OptionSnapshotDaySummary {
    std::optional<double> open{};
    std::optional<double> high{};
    std::optional<double> low{};
    std::optional<double> close{};
    std::optional<double> volume{};
    std::optional<double> change{};
    std::optional<double> change_percent{};
};

void from_json(Json const& j, OptionSnapshotDaySummary& summary);

/// Snapshot payload exposing the latest aggregates for an option contract.
struct OptionSnapshot {
    std::string symbol;
    std::optional<OptionTrade> latest_trade{};
    std::optional<OptionQuote> latest_quote{};
    std::optional<OptionBar> minute_bar{};
    std::optional<OptionBar> daily_bar{};
    std::optional<OptionBar> previous_daily_bar{};
    std::optional<OptionSnapshotDaySummary> day{};
    std::optional<OptionGreeks> greeks{};
    std::optional<OptionRiskParameters> risk_parameters{};
    std::optional<double> open_interest{};
    std::optional<double> implied_volatility{};
};

void from_json(Json const& j, OptionSnapshot& snapshot);

/// Response wrapper containing multi-symbol option snapshots keyed by contract symbol.
struct MultiOptionSnapshots {
    std::map<std::string, OptionSnapshot> snapshots;
};

void from_json(Json const& j, MultiOptionSnapshots& response);

/// Per-contract latest trade envelope mirroring the stock variant.
struct LatestOptionTrade {
    std::string symbol;
    OptionTrade trade;
};

void from_json(Json const& j, LatestOptionTrade& response);

/// Per-contract latest quote envelope mirroring the stock variant.
struct LatestOptionQuote {
    std::string symbol;
    OptionQuote quote;
};

void from_json(Json const& j, LatestOptionQuote& response);

/// Historic option chain record describing a single contract observation.
struct OptionChainEntry {
    std::string symbol;
    std::string underlying_symbol;
    std::string expiration_date;
    std::string strike_price;
    std::string option_type;
    std::optional<OptionGreeks> greeks{};
    std::optional<OptionRiskParameters> risk_parameters{};
    std::optional<OptionQuote> latest_quote{};
    std::optional<OptionTrade> latest_trade{};
    std::optional<double> open_interest{};
};

void from_json(Json const& j, OptionChainEntry& entry);

/// Historical option chain response payload.
struct OptionChain {
    std::string symbol;
    std::vector<OptionChainEntry> contracts;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, OptionChain& response);

/// Response wrapper containing multi-symbol crypto aggregates.
struct MultiCryptoBars {
    std::map<std::string, std::vector<CryptoBar>> bars;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiCryptoBars& response);

/// Response wrapper containing multi-symbol crypto quotes.
struct MultiCryptoQuotes {
    std::map<std::string, std::vector<CryptoQuote>> quotes;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiCryptoQuotes& response);

/// Response wrapper containing multi-symbol crypto trades.
struct MultiCryptoTrades {
    std::map<std::string, std::vector<CryptoTrade>> trades;
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, MultiCryptoTrades& response);

/// Request payload for the news endpoint with typed filters.
struct NewsRequest {
    std::vector<std::string> symbols{};
    std::optional<Timestamp> start{};
    std::optional<Timestamp> end{};
    std::optional<int> limit{};
    std::optional<SortDirection> sort{};
    std::optional<std::string> page_token{};
    bool include_content{false};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Request payload for corporate action announcements.
struct CorporateActionAnnouncementsRequest {
    std::vector<std::string> symbols{};
    std::vector<std::string> corporate_action_types{};
    std::optional<Timestamp> since{};
    std::optional<Timestamp> until{};
    std::optional<int> limit{};
    std::optional<SortDirection> sort{};
    std::optional<std::string> page_token{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Request payload for corporate action events.
struct CorporateActionEventsRequest {
    std::vector<std::string> symbols{};
    std::vector<std::string> corporate_action_types{};
    std::optional<Timestamp> since{};
    std::optional<Timestamp> until{};
    std::optional<int> limit{};
    std::optional<SortDirection> sort{};
    std::optional<std::string> page_token{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Shared request structure for multi-symbol bar endpoints.
struct MultiBarsRequest {
    std::vector<std::string> symbols{};
    std::optional<std::string> timeframe{};
    std::optional<Timestamp> start{};
    std::optional<Timestamp> end{};
    std::optional<int> limit{};
    std::optional<SortDirection> sort{};
    std::optional<std::string> page_token{};
    std::optional<std::string> feed{};
    std::optional<std::string> adjustment{};
    std::optional<Timestamp> asof{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Shared request structure for multi-symbol quote endpoints.
struct MultiQuotesRequest {
    std::vector<std::string> symbols{};
    std::optional<Timestamp> start{};
    std::optional<Timestamp> end{};
    std::optional<int> limit{};
    std::optional<SortDirection> sort{};
    std::optional<std::string> page_token{};
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Shared request structure for multi-symbol trade endpoints.
struct MultiTradesRequest {
    std::vector<std::string> symbols{};
    std::optional<Timestamp> start{};
    std::optional<Timestamp> end{};
    std::optional<int> limit{};
    std::optional<SortDirection> sort{};
    std::optional<std::string> page_token{};
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

using MultiStockBarsRequest = MultiBarsRequest;
using MultiOptionBarsRequest = MultiBarsRequest;
using MultiCryptoBarsRequest = MultiBarsRequest;
using MultiStockQuotesRequest = MultiQuotesRequest;
using MultiOptionQuotesRequest = MultiQuotesRequest;
using MultiCryptoQuotesRequest = MultiQuotesRequest;
using MultiStockTradesRequest = MultiTradesRequest;
using MultiOptionTradesRequest = MultiTradesRequest;
using MultiCryptoTradesRequest = MultiTradesRequest;

/// Request payload for the multi-symbol stock snapshots endpoint.
struct MultiStockSnapshotsRequest {
    std::vector<std::string> symbols{};
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Request payload for single contract option snapshot lookups.
struct OptionSnapshotRequest {
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Request payload for the multi-contract option snapshot endpoint.
struct MultiOptionSnapshotsRequest {
    std::vector<std::string> symbols{};
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Query parameters accepted by the historical option chain endpoint.
struct OptionChainRequest {
    std::optional<std::string> expiration{};
    std::optional<std::string> expiration_gte{};
    std::optional<std::string> expiration_lte{};
    std::optional<std::string> strike{};
    std::optional<std::string> strike_gte{};
    std::optional<std::string> strike_lte{};
    std::optional<std::string> option_type{};
    std::optional<int> limit{};
    std::optional<std::string> page_token{};
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Optional feed override supported by latest option trade lookups.
struct LatestOptionTradeRequest {
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Optional feed override supported by latest option quote lookups.
struct LatestOptionQuoteRequest {
    std::optional<std::string> feed{};

    [[nodiscard]] QueryParams to_query_params() const;
};

} // namespace alpaca
