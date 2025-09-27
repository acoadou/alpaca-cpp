#include "alpaca/MarketDataClient.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "alpaca/HttpClientFactory.hpp"

namespace alpaca {
namespace {
HttpClientPtr ensure_http_client(HttpClientPtr& client) {
    if (!client) {
        client = create_default_http_client();
    }
    return client;
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
} // namespace

MarketDataClient::MarketDataClient(Configuration const& config, HttpClientPtr http_client)
  : v2_client_(config, ensure_http_client(http_client), config.data_base_url),
    beta_client_(config, ensure_http_client(http_client), make_data_beta_base_url(config.data_base_url, "v1beta1")),
    beta_v3_client_(config, ensure_http_client(http_client), make_data_beta_base_url(config.data_base_url, "v1beta3")) {
}

MarketDataClient::MarketDataClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                                   HttpClientPtr http_client)
  : MarketDataClient(Configuration::FromEnvironment(environment, std::move(api_key_id), std::move(api_secret_key)),
                     std::move(http_client)) {
}

LatestStockTrade MarketDataClient::get_latest_stock_trade(std::string const& symbol) const {
    return v2_client_.get<LatestStockTrade>("stocks/" + symbol + "/trades/latest");
}

LatestStockQuote MarketDataClient::get_latest_stock_quote(std::string const& symbol) const {
    return v2_client_.get<LatestStockQuote>("stocks/" + symbol + "/quotes/latest");
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
    return v2_client_.get<LatestStockTrades>("stocks/trades/latest", request.to_query_params());
}

LatestStockQuotes MarketDataClient::get_latest_stock_quotes(LatestStocksRequest const& request) const {
    return v2_client_.get<LatestStockQuotes>("stocks/quotes/latest", request.to_query_params());
}

LatestStockBars MarketDataClient::get_latest_stock_bars(LatestStocksRequest const& request) const {
    return v2_client_.get<LatestStockBars>("stocks/bars/latest", request.to_query_params());
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

namespace {
std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
} // namespace

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
    return v2_client_.get<MultiStockOrderbooks>("stocks/orderbooks", request.to_query_params());
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
    return v2_client_.get<StockBars>("stocks/" + symbol + "/bars", request.to_query_params());
}

std::vector<StockBar> MarketDataClient::get_all_stock_bars(std::string const& symbol, StockBarsRequest request) const {
    std::vector<StockBar> all_bars;
    for (auto const& bar : stock_bars_range(symbol, std::move(request))) {
        all_bars.push_back(bar);
    }
    return all_bars;
}

StockSnapshot MarketDataClient::get_stock_snapshot(std::string const& symbol) const {
    return v2_client_.get<StockSnapshot>("stocks/" + symbol + "/snapshot");
}

MultiStockSnapshots MarketDataClient::get_stock_snapshots(MultiStockSnapshotsRequest const& request) const {
    return v2_client_.get<MultiStockSnapshots>("stocks/snapshots", request.to_query_params());
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
    return v2_client_.get<MultiStockBars>("stocks/bars", request.to_query_params());
}

MultiStockQuotes MarketDataClient::get_stock_quotes(MultiStockQuotesRequest const& request) const {
    return v2_client_.get<MultiStockQuotes>("stocks/quotes", request.to_query_params());
}

MultiStockTrades MarketDataClient::get_stock_trades(MultiStockTradesRequest const& request) const {
    return v2_client_.get<MultiStockTrades>("stocks/trades", request.to_query_params());
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
