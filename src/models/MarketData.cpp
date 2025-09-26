#include "alpaca/models/MarketData.hpp"

namespace alpaca {

namespace {
template <typename T>
std::optional<T> optional_field(const Json& j, const char* key) {
  if (!j.contains(key) || j.at(key).is_null()) {
    return std::nullopt;
  }
  return j.at(key).get<T>();
}

template <typename Item>
void parse_symbol_collection(const Json& j, const char* key,
                             std::map<std::string, std::vector<Item>>& out) {
  out.clear();
  if (!j.contains(key) || !j.at(key).is_object()) {
    return;
  }
  for (const auto& [symbol, value] : j.at(key).items()) {
    out.emplace(symbol, value.template get<std::vector<Item>>());
  }
}
}  // namespace

void from_json(const Json& j, StockTrade& trade) {
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

void from_json(const Json& j, StockQuote& quote) {
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

void from_json(const Json& j, StockBar& bar) {
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

void from_json(const Json& j, LatestStockTrade& response) {
  j.at("symbol").get_to(response.symbol);
  j.at("trade").get_to(response.trade);
}

void from_json(const Json& j, LatestStockQuote& response) {
  j.at("symbol").get_to(response.symbol);
  j.at("quote").get_to(response.quote);
}

void from_json(const Json& j, StockBars& response) {
  j.at("symbol").get_to(response.symbol);
  if (j.contains("bars")) {
    j.at("bars").get_to(response.bars);
  } else {
    response.bars.clear();
  }
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, StockSnapshot& snapshot) {
  j.at("symbol").get_to(snapshot.symbol);
  snapshot.latest_trade = optional_field<StockTrade>(j, "latestTrade");
  snapshot.latest_quote = optional_field<StockQuote>(j, "latestQuote");
  snapshot.minute_bar = optional_field<StockBar>(j, "minuteBar");
  snapshot.daily_bar = optional_field<StockBar>(j, "dailyBar");
  snapshot.previous_daily_bar = optional_field<StockBar>(j, "prevDailyBar");
}

void from_json(const Json& j, MultiStockBars& response) {
  parse_symbol_collection(j, "bars", response.bars);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiStockQuotes& response) {
  parse_symbol_collection(j, "quotes", response.quotes);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiStockTrades& response) {
  parse_symbol_collection(j, "trades", response.trades);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiOptionBars& response) {
  parse_symbol_collection(j, "bars", response.bars);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiOptionQuotes& response) {
  parse_symbol_collection(j, "quotes", response.quotes);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiOptionTrades& response) {
  parse_symbol_collection(j, "trades", response.trades);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiCryptoBars& response) {
  parse_symbol_collection(j, "bars", response.bars);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiCryptoQuotes& response) {
  parse_symbol_collection(j, "quotes", response.quotes);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

void from_json(const Json& j, MultiCryptoTrades& response) {
  parse_symbol_collection(j, "trades", response.trades);
  response.next_page_token = optional_field<std::string>(j, "next_page_token");
}

namespace {
void append_timestamp(QueryParams& params, const std::string& key, const std::optional<Timestamp>& value) {
  if (value.has_value()) {
    params.emplace_back(key, format_timestamp(*value));
  }
}
}  // namespace

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

}  // namespace alpaca

