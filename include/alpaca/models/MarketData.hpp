#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

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

void from_json(const Json& j, StockTrade& trade);

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

void from_json(const Json& j, StockQuote& quote);

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

void from_json(const Json& j, StockBar& bar);

/// Response wrapper containing the latest trade for a symbol.
struct LatestStockTrade {
  std::string symbol;
  StockTrade trade;
};

void from_json(const Json& j, LatestStockTrade& response);

/// Response wrapper containing the latest quote for a symbol.
struct LatestStockQuote {
  std::string symbol;
  StockQuote quote;
};

void from_json(const Json& j, LatestStockQuote& response);

/// Response wrapper containing a collection of bars for a symbol.
struct StockBars {
  std::string symbol;
  std::vector<StockBar> bars;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, StockBars& response);

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

void from_json(const Json& j, StockSnapshot& snapshot);

/// Response wrapper containing multi-symbol stock aggregates.
struct MultiStockBars {
  std::map<std::string, std::vector<StockBar>> bars;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiStockBars& response);

/// Response wrapper containing multi-symbol stock quotes.
struct MultiStockQuotes {
  std::map<std::string, std::vector<StockQuote>> quotes;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiStockQuotes& response);

/// Response wrapper containing multi-symbol stock trades.
struct MultiStockTrades {
  std::map<std::string, std::vector<StockTrade>> trades;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiStockTrades& response);

/// Aliases for option and crypto market data types which share the same schema as equities.
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

void from_json(const Json& j, MultiOptionBars& response);

/// Response wrapper containing multi-symbol option quotes.
struct MultiOptionQuotes {
  std::map<std::string, std::vector<OptionQuote>> quotes;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiOptionQuotes& response);

/// Response wrapper containing multi-symbol option trades.
struct MultiOptionTrades {
  std::map<std::string, std::vector<OptionTrade>> trades;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiOptionTrades& response);

/// Response wrapper containing multi-symbol crypto aggregates.
struct MultiCryptoBars {
  std::map<std::string, std::vector<CryptoBar>> bars;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiCryptoBars& response);

/// Response wrapper containing multi-symbol crypto quotes.
struct MultiCryptoQuotes {
  std::map<std::string, std::vector<CryptoQuote>> quotes;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiCryptoQuotes& response);

/// Response wrapper containing multi-symbol crypto trades.
struct MultiCryptoTrades {
  std::map<std::string, std::vector<CryptoTrade>> trades;
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, MultiCryptoTrades& response);

}  // namespace alpaca

