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

template <typename Item>
void parse_symbol_objects(Json const& j, char const *key, std::map<std::string, Item>& out) {
    out.clear();
    if (!j.contains(key) || !j.at(key).is_object()) {
        return;
    }
    for (auto const& [symbol, value] : j.at(key).items()) {
        out.emplace(symbol, value.template get<Item>());
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

void from_json(Json const& j, MultiStockSnapshots& response) {
    parse_symbol_objects(j, "snapshots", response.snapshots);
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

void from_json(Json const& j, OptionSnapshotDaySummary& summary) {
    summary.open = optional_field<double>(j, "open");
    summary.high = optional_field<double>(j, "high");
    summary.low = optional_field<double>(j, "low");
    summary.close = optional_field<double>(j, "close");
    summary.volume = optional_field<double>(j, "volume");
    summary.change = optional_field<double>(j, "change");
    summary.change_percent = optional_field<double>(j, "changePercent");
}

void from_json(Json const& j, OptionSnapshot& snapshot) {
    if (j.contains("symbol")) {
        snapshot.symbol = j.at("symbol").get<std::string>();
    } else if (j.contains("contract")) {
        snapshot.symbol = j.at("contract").get<std::string>();
    } else {
        snapshot.symbol.clear();
    }
    snapshot.latest_trade = optional_field<OptionTrade>(j, "latestTrade");
    snapshot.latest_quote = optional_field<OptionQuote>(j, "latestQuote");
    snapshot.minute_bar = optional_field<OptionBar>(j, "minuteBar");
    snapshot.daily_bar = optional_field<OptionBar>(j, "dailyBar");
    snapshot.previous_daily_bar = optional_field<OptionBar>(j, "prevDailyBar");
    snapshot.day = optional_field<OptionSnapshotDaySummary>(j, "day");
    snapshot.greeks = optional_field<OptionGreeks>(j, "greeks");
    snapshot.risk_parameters = optional_field<OptionRiskParameters>(j, "riskParameters");
    snapshot.open_interest = optional_field<double>(j, "openInterest");
    snapshot.implied_volatility = optional_field<double>(j, "impliedVolatility");
}

void from_json(Json const& j, MultiOptionSnapshots& response) {
    parse_symbol_objects(j, "snapshots", response.snapshots);
}

void from_json(Json const& j, LatestOptionTrade& response) {
    j.at("symbol").get_to(response.symbol);
    j.at("trade").get_to(response.trade);
}

void from_json(Json const& j, LatestOptionQuote& response) {
    j.at("symbol").get_to(response.symbol);
    j.at("quote").get_to(response.quote);
}

void from_json(Json const& j, OptionChainEntry& entry) {
    if (j.contains("symbol")) {
        entry.symbol = j.at("symbol").get<std::string>();
    } else if (j.contains("contract")) {
        entry.symbol = j.at("contract").get<std::string>();
    } else {
        entry.symbol.clear();
    }
    if (j.contains("underlying_symbol")) {
        entry.underlying_symbol = j.at("underlying_symbol").get<std::string>();
    } else if (j.contains("underlyingSymbol")) {
        entry.underlying_symbol = j.at("underlyingSymbol").get<std::string>();
    } else {
        entry.underlying_symbol.clear();
    }
    if (j.contains("expiration")) {
        entry.expiration_date = j.at("expiration").get<std::string>();
    } else if (j.contains("expiration_date")) {
        entry.expiration_date = j.at("expiration_date").get<std::string>();
    } else {
        entry.expiration_date.clear();
    }
    if (j.contains("strike")) {
        entry.strike_price = j.at("strike").get<std::string>();
    } else if (j.contains("strike_price")) {
        entry.strike_price = j.at("strike_price").get<std::string>();
    } else {
        entry.strike_price.clear();
    }
    if (j.contains("option_type")) {
        entry.option_type = j.at("option_type").get<std::string>();
    } else if (j.contains("type")) {
        entry.option_type = j.at("type").get<std::string>();
    } else {
        entry.option_type.clear();
    }
    entry.greeks = optional_field<OptionGreeks>(j, "greeks");
    entry.risk_parameters = optional_field<OptionRiskParameters>(j, "riskParameters");
    entry.latest_quote = optional_field<OptionQuote>(j, "latestQuote");
    entry.latest_trade = optional_field<OptionTrade>(j, "latestTrade");
    entry.open_interest = optional_field<double>(j, "openInterest");
}

void from_json(Json const& j, OptionChain& response) {
    if (j.contains("symbol")) {
        response.symbol = j.at("symbol").get<std::string>();
    } else if (j.contains("underlying_symbol")) {
        response.symbol = j.at("underlying_symbol").get<std::string>();
    } else if (j.contains("underlyingSymbol")) {
        response.symbol = j.at("underlyingSymbol").get<std::string>();
    } else {
        response.symbol.clear();
    }
    if (j.contains("contracts")) {
        response.contracts = j.at("contracts").get<std::vector<OptionChainEntry>>();
    } else if (j.contains("items")) {
        response.contracts = j.at("items").get<std::vector<OptionChainEntry>>();
    } else {
        response.contracts.clear();
    }
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

void from_json(Json const& j, OrderbookQuote& quote) {
    if (j.contains("p")) {
        j.at("p").get_to(quote.price);
    } else if (j.contains("price")) {
        j.at("price").get_to(quote.price);
    } else {
        quote.price = 0.0;
    }
    if (j.contains("s")) {
        j.at("s").get_to(quote.size);
    } else if (j.contains("size")) {
        j.at("size").get_to(quote.size);
    } else {
        quote.size = 0.0;
    }
    if (j.contains("x") && !j.at("x").is_null()) {
        quote.exchange = j.at("x").get<std::string>();
    } else if (j.contains("exchange") && !j.at("exchange").is_null()) {
        quote.exchange = j.at("exchange").get<std::string>();
    } else {
        quote.exchange = std::nullopt;
    }
}

void from_json(Json const& j, OrderbookSnapshot& snapshot) {
    if (j.contains("symbol")) {
        snapshot.symbol = j.at("symbol").get<std::string>();
    } else {
        snapshot.symbol.clear();
    }
    if (j.contains("t")) {
        snapshot.timestamp = parse_timestamp(j.at("t").get<std::string>());
    } else if (j.contains("timestamp")) {
        snapshot.timestamp = parse_timestamp(j.at("timestamp").get<std::string>());
    } else {
        snapshot.timestamp = {};
    }
    if (j.contains("b")) {
        snapshot.bids = j.at("b").get<std::vector<OrderbookQuote>>();
    } else if (j.contains("bids")) {
        snapshot.bids = j.at("bids").get<std::vector<OrderbookQuote>>();
    } else {
        snapshot.bids.clear();
    }
    if (j.contains("a")) {
        snapshot.asks = j.at("a").get<std::vector<OrderbookQuote>>();
    } else if (j.contains("asks")) {
        snapshot.asks = j.at("asks").get<std::vector<OrderbookQuote>>();
    } else {
        snapshot.asks.clear();
    }
    if (j.contains("r")) {
        snapshot.reset = j.at("r").get<bool>();
    } else if (j.contains("reset")) {
        snapshot.reset = j.at("reset").get<bool>();
    } else {
        snapshot.reset = false;
    }
}

void from_json(Json const& j, LatestStockTrades& response) {
    parse_symbol_objects(j, "trades", response.trades);
}

void from_json(Json const& j, LatestStockQuotes& response) {
    parse_symbol_objects(j, "quotes", response.quotes);
}

void from_json(Json const& j, LatestStockBars& response) {
    parse_symbol_objects(j, "bars", response.bars);
}

void from_json(Json const& j, LatestOptionTrades& response) {
    parse_symbol_objects(j, "trades", response.trades);
}

void from_json(Json const& j, LatestOptionQuotes& response) {
    parse_symbol_objects(j, "quotes", response.quotes);
}

void from_json(Json const& j, LatestOptionBars& response) {
    parse_symbol_objects(j, "bars", response.bars);
}

void from_json(Json const& j, LatestCryptoTrades& response) {
    parse_symbol_objects(j, "trades", response.trades);
}

void from_json(Json const& j, LatestCryptoQuotes& response) {
    parse_symbol_objects(j, "quotes", response.quotes);
}

void from_json(Json const& j, LatestCryptoBars& response) {
    parse_symbol_objects(j, "bars", response.bars);
}

namespace {
template <typename Orderbooks>
void parse_orderbooks(Json const& j, Orderbooks& response) {
    response.orderbooks.clear();
    if (!j.contains("orderbooks") || !j.at("orderbooks").is_object()) {
        return;
    }
    for (auto const& [symbol, value] : j.at("orderbooks").items()) {
        OrderbookSnapshot snapshot = value.get<OrderbookSnapshot>();
        snapshot.symbol = symbol;
        response.orderbooks.emplace(symbol, std::move(snapshot));
    }
}
} // namespace

void from_json(Json const& j, MultiStockOrderbooks& response) {
    parse_orderbooks(j, response);
}

void from_json(Json const& j, MultiOptionOrderbooks& response) {
    parse_orderbooks(j, response);
}

void from_json(Json const& j, MultiCryptoOrderbooks& response) {
    parse_orderbooks(j, response);
}

void from_json(Json const& j, Exchange& exchange) {
    if (j.contains("id")) {
        j.at("id").get_to(exchange.id);
    } else if (j.contains("exchange")) {
        j.at("exchange").get_to(exchange.id);
    } else if (j.contains("code")) {
        j.at("code").get_to(exchange.id);
    } else {
        exchange.id.clear();
    }
    if (j.contains("name")) {
        j.at("name").get_to(exchange.name);
    } else {
        exchange.name.clear();
    }
    exchange.code = optional_field<std::string>(j, "code");
    exchange.country = optional_field<std::string>(j, "country");
    exchange.currency = optional_field<std::string>(j, "currency");
    exchange.timezone = optional_field<std::string>(j, "timezone");
    exchange.mic = optional_field<std::string>(j, "mic");
    exchange.operating_mic = optional_field<std::string>(j, "operating_mic");
}

void from_json(Json const& j, TradeCondition& condition) {
    if (j.contains("id")) {
        j.at("id").get_to(condition.id);
    } else if (j.contains("code")) {
        j.at("code").get_to(condition.id);
    } else {
        condition.id.clear();
    }
    if (j.contains("name")) {
        j.at("name").get_to(condition.name);
    } else if (j.contains("description")) {
        condition.name = j.at("description").get<std::string>();
    } else {
        condition.name.clear();
    }
    condition.description = optional_field<std::string>(j, "description");
    condition.type = optional_field<std::string>(j, "type");
}

void from_json(Json const& j, ListExchangesResponse& response) {
    if (j.contains("exchanges")) {
        j.at("exchanges").get_to(response.exchanges);
    } else {
        response.exchanges.clear();
    }
}

void from_json(Json const& j, ListTradeConditionsResponse& response) {
    if (j.contains("conditions")) {
        j.at("conditions").get_to(response.conditions);
    } else {
        response.conditions.clear();
    }
}

void from_json(Json const& j, MarketMover& mover) {
    j.at("symbol").get_to(mover.symbol);
    if (j.contains("percent_change")) {
        j.at("percent_change").get_to(mover.percent_change);
    } else if (j.contains("percentage_change")) {
        j.at("percentage_change").get_to(mover.percent_change);
    } else if (j.contains("percentChange")) {
        j.at("percentChange").get_to(mover.percent_change);
    }
    if (j.contains("change")) {
        j.at("change").get_to(mover.change);
    } else if (j.contains("price_change")) {
        j.at("price_change").get_to(mover.change);
    }
    if (j.contains("price")) {
        mover.price = j.at("price").get<double>();
    } else if (j.contains("last_price")) {
        mover.price = j.at("last_price").get<double>();
    } else if (j.contains("current_price")) {
        mover.price = j.at("current_price").get<double>();
    } else {
        mover.price = 0.0;
    }
}

void from_json(Json const& j, MarketMoversResponse& response) {
    if (j.contains("gainers")) {
        j.at("gainers").get_to(response.gainers);
    } else {
        response.gainers.clear();
    }
    if (j.contains("losers")) {
        j.at("losers").get_to(response.losers);
    } else {
        response.losers.clear();
    }
    if (j.contains("market_type")) {
        j.at("market_type").get_to(response.market_type);
    } else if (j.contains("marketType")) {
        j.at("marketType").get_to(response.market_type);
    } else {
        response.market_type.clear();
    }
    if (j.contains("last_updated")) {
        response.last_updated = parse_timestamp(j.at("last_updated").get<std::string>());
    } else if (j.contains("lastUpdated")) {
        response.last_updated = parse_timestamp(j.at("lastUpdated").get<std::string>());
    } else {
        response.last_updated = {};
    }
}

void from_json(Json const& j, MostActiveStock& stock) {
    j.at("symbol").get_to(stock.symbol);
    if (j.contains("volume")) {
        j.at("volume").get_to(stock.volume);
    } else {
        stock.volume = 0.0;
    }
    if (j.contains("trade_count")) {
        j.at("trade_count").get_to(stock.trade_count);
    } else if (j.contains("tradeCount")) {
        j.at("tradeCount").get_to(stock.trade_count);
    } else {
        stock.trade_count = 0.0;
    }
}

void from_json(Json const& j, MostActiveStocksResponse& response) {
    if (j.contains("most_actives")) {
        j.at("most_actives").get_to(response.most_actives);
    } else if (j.contains("mostActives")) {
        j.at("mostActives").get_to(response.most_actives);
    } else if (j.contains("most_active")) {
        j.at("most_active").get_to(response.most_actives);
    } else {
        response.most_actives.clear();
    }
    if (j.contains("last_updated")) {
        response.last_updated = parse_timestamp(j.at("last_updated").get<std::string>());
    } else if (j.contains("lastUpdated")) {
        response.last_updated = parse_timestamp(j.at("lastUpdated").get<std::string>());
    } else {
        response.last_updated = {};
    }
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

QueryParams MultiStockSnapshotsRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams OptionSnapshotRequest::to_query_params() const {
    QueryParams params;
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams MultiOptionSnapshotsRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams OptionChainRequest::to_query_params() const {
    QueryParams params;
    if (expiration.has_value()) {
        params.emplace_back("expiration", *expiration);
    }
    if (expiration_gte.has_value()) {
        params.emplace_back("expiration_gte", *expiration_gte);
    }
    if (expiration_lte.has_value()) {
        params.emplace_back("expiration_lte", *expiration_lte);
    }
    if (strike.has_value()) {
        params.emplace_back("strike", *strike);
    }
    if (strike_gte.has_value()) {
        params.emplace_back("strike_gte", *strike_gte);
    }
    if (strike_lte.has_value()) {
        params.emplace_back("strike_lte", *strike_lte);
    }
    if (option_type.has_value()) {
        params.emplace_back("type", *option_type);
    }
    append_limit(params, limit);
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams LatestOptionTradeRequest::to_query_params() const {
    QueryParams params;
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams LatestOptionQuoteRequest::to_query_params() const {
    QueryParams params;
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams LatestStocksRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    if (currency.has_value()) {
        params.emplace_back("currency", *currency);
    }
    return params;
}

QueryParams LatestOptionsRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams LatestCryptoRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    return params;
}

QueryParams LatestStockOrderbooksRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    if (currency.has_value()) {
        params.emplace_back("currency", *currency);
    }
    return params;
}

QueryParams LatestOptionOrderbooksRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    if (feed.has_value()) {
        params.emplace_back("feed", *feed);
    }
    return params;
}

QueryParams LatestCryptoOrderbooksRequest::to_query_params() const {
    validate_symbols(symbols);
    QueryParams params;
    append_csv(params, "symbols", symbols);
    return params;
}

QueryParams ListExchangesRequest::to_query_params() const {
    QueryParams params;
    if (locale.has_value()) {
        params.emplace_back("locale", *locale);
    }
    if (region.has_value()) {
        params.emplace_back("region", *region);
    }
    if (mic.has_value()) {
        params.emplace_back("mic", *mic);
    }
    return params;
}

QueryParams ListTradeConditionsRequest::to_query_params() const {
    QueryParams params;
    if (sip.has_value()) {
        params.emplace_back("sip", *sip);
    }
    return params;
}

QueryParams MarketMoversRequest::to_query_params() const {
    QueryParams params;
    if (top.has_value()) {
        if (*top <= 0) {
            throw std::invalid_argument("top must be positive");
        }
        params.emplace_back("top", std::to_string(*top));
    }
    return params;
}

QueryParams MostActiveStocksRequest::to_query_params() const {
    QueryParams params;
    if (by.empty()) {
        throw std::invalid_argument("by must not be empty");
    }
    params.emplace_back("by", by);
    if (top.has_value()) {
        if (*top <= 0) {
            throw std::invalid_argument("top must be positive");
        }
        params.emplace_back("top", std::to_string(*top));
    }
    return params;
}

} // namespace alpaca
