#include "alpaca/MarketDataClient.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

#include "alpaca/HttpClientFactory.hpp"

namespace alpaca {
namespace {
std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string make_data_beta_base_url(std::string const& data_base_url, std::string_view version) {
    constexpr std::string_view v2_suffix{"/v2"};
    if (data_base_url.ends_with(v2_suffix)) {
        return std::string(data_base_url.substr(0, data_base_url.size() - v2_suffix.size())) + '/' +
            std::string(version);
    }
    if (!data_base_url.empty() && data_base_url.back() == '/') {
        return data_base_url + std::string(version);
    }
    return data_base_url + '/' + std::string(version);
}
bool path_contains_segment(std::string const& path, std::string_view segment) {
    if (segment.empty()) {
        return false;
    }
    std::string const needle = '/' + std::string(segment);
    auto pos = path.find(needle);
    while (pos != std::string::npos) {
        auto const after = pos + needle.size();
        if (after == path.size() || path[after] == '/' || path[after] == '?' || path[after] == '#') {
            return true;
        }
        pos = path.find(needle, pos + 1);
    }
    return false;
}

std::optional<MarketDataPlan> plan_hint_from_url(std::string const& url) {
    if (url.empty()) {
        return std::nullopt;
    }
    std::string lower = to_lower_copy(url);
    auto scheme_pos = lower.find("//");
    std::size_t host_start = 0;
    if (scheme_pos != std::string::npos) {
        host_start = scheme_pos + 2;
    }
    auto path_start = lower.find('/', host_start);
    std::string host = lower.substr(host_start, path_start == std::string::npos ? std::string::npos : path_start - host_start);
    std::string path = path_start == std::string::npos ? std::string{} : lower.substr(path_start);

    auto contains_feed_token = [](std::string const& value, std::string_view token) {
        if (value.empty()) {
            return false;
        }
        std::string needle = '.' + std::string(token) + '.';
        if (value.find(needle) != std::string::npos) {
            return true;
        }
        needle = std::string(token) + '.';
        if (value.rfind(needle, 0) == 0) {
            return true;
        }
        needle = '.' + std::string(token);
        if (!value.empty() && value.rfind(needle) == value.size() - needle.size()) {
            return true;
        }
        needle = '-' + std::string(token) + '.';
        if (value.find(needle) != std::string::npos) {
            return true;
        }
        return false;
    };

    if (contains_feed_token(host, "sip") || path_contains_segment(path, "sip")) {
        return MarketDataPlan::SIP;
    }
    if (contains_feed_token(host, "iex") || path_contains_segment(path, "iex")) {
        return MarketDataPlan::IEX;
    }
    return std::nullopt;
}

std::optional<MarketDataPlan> detect_plan_hint(Configuration const& config) {
    if (auto plan = plan_hint_from_url(config.data_base_url)) {
        return plan;
    }
    if (auto plan = plan_hint_from_url(config.market_data_stream_url)) {
        return plan;
    }
    return std::nullopt;
}

std::string_view plan_feed_name(MarketDataPlan plan) {
    switch (plan) {
    case MarketDataPlan::IEX:
        return "iex";
    case MarketDataPlan::SIP:
        return "sip";
    case MarketDataPlan::Auto:
        break;
    }
    return "iex";
}

MarketDataPlan resolve_market_data_plan(Configuration const& config) {
    auto hint = detect_plan_hint(config);
    switch (config.market_data_plan) {
    case MarketDataPlan::IEX:
        if (hint.has_value() && *hint == MarketDataPlan::SIP) {
            throw std::invalid_argument(
                "market_data_plan is set to IEX but configuration URLs reference the SIP feed");
        }
        return MarketDataPlan::IEX;
    case MarketDataPlan::SIP:
        if (hint.has_value() && *hint == MarketDataPlan::IEX) {
            throw std::invalid_argument(
                "market_data_plan is set to SIP but configuration URLs reference the IEX feed");
        }
        return MarketDataPlan::SIP;
    case MarketDataPlan::Auto:
        if (hint.has_value()) {
            return *hint;
        }
        return MarketDataPlan::IEX;
    }
    return MarketDataPlan::IEX;
}

template <typename T, typename = void> struct has_feed_member : std::false_type {};

template <typename T>
struct has_feed_member<T, std::void_t<decltype(std::declval<T&>().feed)>> : std::true_type {};

template <typename T> inline constexpr bool has_feed_member_v = has_feed_member<T>::value;

template <typename Request>
Request prepare_stock_request(Request request, MarketDataPlan plan, std::string const& default_feed) {
    if constexpr (has_feed_member_v<Request>) {
        using FeedType = std::decay_t<decltype(request.feed)>;
        if constexpr (std::is_same_v<FeedType, std::optional<std::string>>) {
            if (request.feed.has_value()) {
                auto normalized = to_lower_copy(*request.feed);
                if (plan == MarketDataPlan::IEX && normalized == "sip") {
                    throw std::invalid_argument(
                        "SIP market data feed requires the SIP data plan. "
                        "Update Configuration::market_data_plan to MarketDataPlan::SIP or adjust the request feed.");
                }
                request.feed = std::move(normalized);
            } else {
                request.feed = default_feed;
            }
        }
    }
    return request;
}
} // namespace

MarketDataClient::MarketDataClient(Configuration const& config, HttpClientPtr http_client, RestClient::Options options)
    : stock_data_plan_(resolve_market_data_plan(config)),
    stock_data_feed_(std::string(plan_feed_name(stock_data_plan_))),
    v2_client_(config, ensure_http_client(http_client), config.data_base_url, options),
    beta_client_(config,
                 ensure_http_client(http_client),
                 make_data_beta_base_url(config.data_base_url, "v1beta1"),
                 options),
    beta_v3_client_(config,
                    ensure_http_client(http_client),
                    make_data_beta_base_url(config.data_base_url, "v1beta3"),
                    std::move(options)) {}

MarketDataClient::MarketDataClient(Configuration const& config, RestClient::Options options)
    : MarketDataClient(config, nullptr, std::move(options)) {}

MarketDataClient::MarketDataClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                                   HttpClientPtr http_client, RestClient::Options options)
    : MarketDataClient(Configuration::FromEnvironment(environment, std::move(api_key_id), std::move(api_secret_key)),
                       std::move(http_client),
                       std::move(options)) {}

MarketDataClient::MarketDataClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                                   RestClient::Options options)
    : MarketDataClient(environment,
                       std::move(api_key_id),
                       std::move(api_secret_key),
                       nullptr,
                       std::move(options)) {}

LatestStockTrade MarketDataClient::get_latest_stock_trade(std::string const& symbol) const {
    QueryParams params{{"feed", stock_data_feed_}};
    return v2_client_.get<LatestStockTrade>("stocks/" + symbol + "/trades/latest", params);
}

LatestStockQuote MarketDataClient::get_latest_stock_quote(std::string const& symbol) const {
    QueryParams params{{"feed", stock_data_feed_}};
    return v2_client_.get<LatestStockQuote>("stocks/" + symbol + "/quotes/latest", params);
}

LatestOptionTrade MarketDataClient::get_latest_option_trade(std::string const& symbol,
                                                            LatestOptionTradeRequest const& request) const {
    return beta_client_.get<LatestOptionTrade>("options/" + symbol + "/trades/latest", request.to_query_params());
}

LatestOptionQuote MarketDataClient::get_latest_option_quote(std::string const& symbol,
                                                            LatestOptionQuoteRequest const& request) const {
    return beta_client_.get<LatestOptionQuote>("options/" + symbol + "/quotes/latest", request.to_query_params());
}

LatestStockTrades MarketDataClient::get_latest_stock_trades(LatestStocksRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<LatestStockTrades>("stocks/trades/latest", effective.to_query_params());
}

LatestStockQuotes MarketDataClient::get_latest_stock_quotes(LatestStocksRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<LatestStockQuotes>("stocks/quotes/latest", effective.to_query_params());
}

LatestStockBars MarketDataClient::get_latest_stock_bars(LatestStocksRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<LatestStockBars>("stocks/bars/latest", effective.to_query_params());
}

LatestOptionTrades MarketDataClient::get_latest_option_trades(LatestOptionsRequest const& request) const {
    return beta_client_.get<LatestOptionTrades>("options/trades/latest", request.to_query_params());
}

LatestOptionQuotes MarketDataClient::get_latest_option_quotes(LatestOptionsRequest const& request) const {
    return beta_client_.get<LatestOptionQuotes>("options/quotes/latest", request.to_query_params());
}

LatestOptionBars MarketDataClient::get_latest_option_bars(LatestOptionsRequest const& request) const {
    return beta_client_.get<LatestOptionBars>("options/bars/latest", request.to_query_params());
}

LatestCryptoTrades MarketDataClient::get_latest_crypto_trades(std::string const& feed,
                                                              LatestCryptoRequest const& request) const {
    if (feed.empty()) {
        throw std::invalid_argument("feed must not be empty");
    }
    return beta_client_.get<LatestCryptoTrades>("crypto/" + to_lower_copy(feed) + "/latest/trades",
                                                request.to_query_params());
}

LatestCryptoQuotes MarketDataClient::get_latest_crypto_quotes(std::string const& feed,
                                                              LatestCryptoRequest const& request) const {
    if (feed.empty()) {
        throw std::invalid_argument("feed must not be empty");
    }
    return beta_client_.get<LatestCryptoQuotes>("crypto/" + to_lower_copy(feed) + "/latest/quotes",
                                                request.to_query_params());
}

LatestCryptoBars MarketDataClient::get_latest_crypto_bars(std::string const& feed,
                                                          LatestCryptoRequest const& request) const {
    if (feed.empty()) {
        throw std::invalid_argument("feed must not be empty");
    }
    return beta_client_.get<LatestCryptoBars>("crypto/" + to_lower_copy(feed) + "/latest/bars",
                                              request.to_query_params());
}

MultiStockOrderbooks MarketDataClient::get_stock_orderbooks(LatestStockOrderbooksRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<MultiStockOrderbooks>("stocks/orderbooks", effective.to_query_params());
}

MultiOptionOrderbooks MarketDataClient::get_option_orderbooks(LatestOptionOrderbooksRequest const& request) const {
    return beta_client_.get<MultiOptionOrderbooks>("options/orderbooks", request.to_query_params());
}

MultiCryptoOrderbooks MarketDataClient::get_crypto_orderbooks(std::string const& feed,
                                                              LatestCryptoOrderbooksRequest const& request) const {
    if (feed.empty()) {
        throw std::invalid_argument("feed must not be empty");
    }
    return beta_client_.get<MultiCryptoOrderbooks>("crypto/" + to_lower_copy(feed) + "/latest/orderbooks",
                                                   request.to_query_params());
}

StockBars MarketDataClient::get_stock_bars(std::string const& symbol, StockBarsRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<StockBars>("stocks/" + symbol + "/bars", effective.to_query_params());
}

std::vector<StockBar> MarketDataClient::get_all_stock_bars(std::string const& symbol, StockBarsRequest request) const {
    std::vector<StockBar> all_bars;
    for (auto const& bar : stock_bars_range(symbol, std::move(request))) {
        all_bars.push_back(bar);
    }
    return all_bars;
}

StockSnapshot MarketDataClient::get_stock_snapshot(std::string const& symbol) const {
    QueryParams params{{"feed", stock_data_feed_}};
    return v2_client_.get<StockSnapshot>("stocks/" + symbol + "/snapshot", params);
}

MultiStockSnapshots MarketDataClient::get_stock_snapshots(MultiStockSnapshotsRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<MultiStockSnapshots>("stocks/snapshots", effective.to_query_params());
}

CryptoSnapshot MarketDataClient::get_crypto_snapshot(std::string const& feed, std::string const& symbol,
                                                     CryptoSnapshotRequest const& request) const {
    if (feed.empty()) {
        throw std::invalid_argument("feed must not be empty");
    }
    return beta_v3_client_.get<CryptoSnapshot>("crypto/" + to_lower_copy(feed) + "/snapshots/" + symbol,
                                               request.to_query_params());
}

MultiCryptoSnapshots MarketDataClient::get_crypto_snapshots(std::string const& feed,
                                                            MultiCryptoSnapshotsRequest const& request) const {
    if (feed.empty()) {
        throw std::invalid_argument("feed must not be empty");
    }
    return beta_v3_client_.get<MultiCryptoSnapshots>("crypto/" + to_lower_copy(feed) + "/snapshots",
                                                     request.to_query_params());
}

PaginatedVectorRange<StockBarsRequest, StockBars, StockBar>
MarketDataClient::stock_bars_range(std::string const& symbol, StockBarsRequest request) const {
    return PaginatedVectorRange<StockBarsRequest, StockBars, StockBar>(
        std::move(request),
        [this, symbol](StockBarsRequest const& req) {
        return get_stock_bars(symbol, req);
    },
        [](StockBars const& page) -> std::vector<StockBar> const& {
        return page.bars;
    },
        [](StockBars const& page) {
        return page.next_page_token;
    },
        [](StockBarsRequest& req, std::optional<std::string> const& token) {
        req.page_token = token;
    });
}

NewsResponse MarketDataClient::get_news(NewsRequest const& request) const {
    return beta_client_.get<NewsResponse>("news", request.to_query_params());
}

PaginatedVectorRange<NewsRequest, NewsResponse, NewsArticle> MarketDataClient::news_range(NewsRequest request) const {
    return PaginatedVectorRange<NewsRequest, NewsResponse, NewsArticle>(
        std::move(request),
        [this](NewsRequest const& req) {
        return get_news(req);
    },
        [](NewsResponse const& page) -> std::vector<NewsArticle> const& {
        return page.news;
    },
        [](NewsResponse const& page) {
        return page.next_page_token;
    },
        [](NewsRequest& req, std::optional<std::string> const& token) {
        req.page_token = token;
    });
}

HistoricalAuctionsResponse MarketDataClient::get_stock_auctions(std::string const& symbol,
                                                                HistoricalAuctionsRequest const& request) const {
    return v2_client_.get<HistoricalAuctionsResponse>("stocks/" + symbol + "/auctions", request.to_query_params());
}

HistoricalAuctionsResponse MarketDataClient::get_auctions(HistoricalAuctionsRequest const& request) const {
    return v2_client_.get<HistoricalAuctionsResponse>("stocks/auctions", request.to_query_params());
}

PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>
MarketDataClient::stock_auctions_range(std::string const& symbol, HistoricalAuctionsRequest request) const {
    return PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>(
        std::move(request),
        [this, symbol](HistoricalAuctionsRequest const& req) {
        return get_stock_auctions(symbol, req);
    },
        [](HistoricalAuctionsResponse const& page) -> std::vector<StockAuction> const& {
        return page.auctions;
    },
        [](HistoricalAuctionsResponse const& page) {
        return page.next_page_token;
    },
        [](HistoricalAuctionsRequest& req, std::optional<std::string> const& token) {
        req.page_token = token;
    });
}

PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>
MarketDataClient::auctions_range(HistoricalAuctionsRequest request) const {
    return PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>(
        std::move(request),
        [this](HistoricalAuctionsRequest const& req) {
        return get_auctions(req);
    },
        [](HistoricalAuctionsResponse const& page) -> std::vector<StockAuction> const& {
        return page.auctions;
    },
        [](HistoricalAuctionsResponse const& page) {
        return page.next_page_token;
    },
        [](HistoricalAuctionsRequest& req, std::optional<std::string> const& token) {
        req.page_token = token;
    });
}

CorporateActionAnnouncementsResponse
MarketDataClient::get_corporate_announcements(CorporateActionAnnouncementsRequest const& request) const {
    return beta_client_.get<CorporateActionAnnouncementsResponse>("corporate-actions/announcements",
                                                                  request.to_query_params());
}

CorporateActionEventsResponse
MarketDataClient::get_corporate_actions(CorporateActionEventsRequest const& request) const {
    return beta_client_.get<CorporateActionEventsResponse>("corporate-actions/events", request.to_query_params());
}

MultiStockBars MarketDataClient::get_stock_aggregates(MultiStockBarsRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<MultiStockBars>("stocks/bars", effective.to_query_params());
}

MultiStockQuotes MarketDataClient::get_stock_quotes(MultiStockQuotesRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<MultiStockQuotes>("stocks/quotes", effective.to_query_params());
}

MultiStockTrades MarketDataClient::get_stock_trades(MultiStockTradesRequest const& request) const {
    auto effective = prepare_stock_request(request, stock_data_plan_, stock_data_feed_);
    return v2_client_.get<MultiStockTrades>("stocks/trades", effective.to_query_params());
}

MultiOptionBars MarketDataClient::get_option_aggregates(MultiOptionBarsRequest const& request) const {
    return beta_client_.get<MultiOptionBars>("options/bars", request.to_query_params());
}

MultiOptionQuotes MarketDataClient::get_option_quotes(MultiOptionQuotesRequest const& request) const {
    return beta_client_.get<MultiOptionQuotes>("options/quotes", request.to_query_params());
}

MultiOptionTrades MarketDataClient::get_option_trades(MultiOptionTradesRequest const& request) const {
    return beta_client_.get<MultiOptionTrades>("options/trades", request.to_query_params());
}

OptionSnapshot MarketDataClient::get_option_snapshot(std::string const& symbol,
                                                     OptionSnapshotRequest const& request) const {
    return beta_client_.get<OptionSnapshot>("options/" + symbol + "/snapshot", request.to_query_params());
}

MultiOptionSnapshots MarketDataClient::get_option_snapshots(MultiOptionSnapshotsRequest const& request) const {
    return beta_client_.get<MultiOptionSnapshots>("options/snapshots", request.to_query_params());
}

OptionChain MarketDataClient::get_option_chain(std::string const& symbol, OptionChainRequest const& request) const {
    return beta_client_.get<OptionChain>("options/" + symbol + "/chain", request.to_query_params());
}

MultiCryptoBars MarketDataClient::get_crypto_aggregates(MultiCryptoBarsRequest const& request) const {
    return beta_client_.get<MultiCryptoBars>("crypto/bars", request.to_query_params());
}

MultiCryptoQuotes MarketDataClient::get_crypto_quotes(MultiCryptoQuotesRequest const& request) const {
    return beta_client_.get<MultiCryptoQuotes>("crypto/quotes", request.to_query_params());
}

MultiCryptoTrades MarketDataClient::get_crypto_trades(MultiCryptoTradesRequest const& request) const {
    return beta_client_.get<MultiCryptoTrades>("crypto/trades", request.to_query_params());
}

LatestCryptoTrades MarketDataClient::get_latest_crypto_trade(std::string const& feed,
                                                             LatestCryptoDataRequest const& request) const {
    return beta_v3_client_.get<LatestCryptoTrades>("crypto/" + feed + "/latest/trades", request.to_query_params());
}

LatestCryptoQuotes MarketDataClient::get_latest_crypto_quote(std::string const& feed,
                                                             LatestCryptoDataRequest const& request) const {
    return beta_v3_client_.get<LatestCryptoQuotes>("crypto/" + feed + "/latest/quotes", request.to_query_params());
}

LatestCryptoBars MarketDataClient::get_latest_crypto_bar(std::string const& feed,
                                                         LatestCryptoDataRequest const& request) const {
    return beta_v3_client_.get<LatestCryptoBars>("crypto/" + feed + "/latest/bars", request.to_query_params());
}

LatestCryptoOrderbooks
MarketDataClient::get_latest_crypto_orderbook(std::string const& feed,
                                              LatestCryptoOrderbookRequest const& request) const {
    return beta_v3_client_.get<LatestCryptoOrderbooks>("crypto/" + feed + "/latest/orderbooks",
                                                       request.to_query_params());
}

ListExchangesResponse MarketDataClient::list_exchanges(ListExchangesRequest const& request) const {
    if (request.asset_class.empty()) {
        throw std::invalid_argument("asset_class must not be empty");
    }
    return v2_client_.get<ListExchangesResponse>("meta/exchanges/" + to_lower_copy(request.asset_class),
                                                 request.to_query_params());
}

ListTradeConditionsResponse MarketDataClient::list_trade_conditions(ListTradeConditionsRequest const& request) const {
    if (request.asset_class.empty()) {
        throw std::invalid_argument("asset_class must not be empty");
    }
    if (request.condition_type.empty()) {
        throw std::invalid_argument("condition_type must not be empty");
    }
    std::string asset_class = to_lower_copy(request.asset_class);
    std::string condition_type = to_lower_copy(request.condition_type);
    return v2_client_.get<ListTradeConditionsResponse>("meta/conditions/" + asset_class + "/" + condition_type,
                                                       request.to_query_params());
}

MarketMoversResponse MarketDataClient::get_top_market_movers(MarketMoversRequest const& request) const {
    if (request.market_type.empty()) {
        throw std::invalid_argument("market_type must not be empty");
    }
    std::string market_type = to_lower_copy(request.market_type);
    return beta_client_.get<MarketMoversResponse>("screener/" + market_type + "/movers", request.to_query_params());
}

MostActiveStocksResponse MarketDataClient::get_most_active_stocks(MostActiveStocksRequest const& request) const {
    return beta_client_.get<MostActiveStocksResponse>("screener/stocks/most-actives", request.to_query_params());
}

} // namespace alpaca
