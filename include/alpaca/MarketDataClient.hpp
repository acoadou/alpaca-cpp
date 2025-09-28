#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Configuration.hpp"
#include "alpaca/Pagination.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/CorporateActions.hpp"
#include "alpaca/models/MarketData.hpp"
#include "alpaca/models/News.hpp"

namespace alpaca {

/// Market data client surfaces historical and real-time REST endpoints.
class MarketDataClient {
  public:
    MarketDataClient(Configuration const& config, HttpClientPtr http_client = nullptr,
                     RestClient::Options options = RestClient::default_options());
    MarketDataClient(Configuration const& config, RestClient::Options options);
    MarketDataClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                     HttpClientPtr http_client = nullptr, RestClient::Options options = RestClient::default_options());
    MarketDataClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                     RestClient::Options options);

    [[nodiscard]] LatestStockTrade get_latest_stock_trade(std::string const& symbol) const;
    [[nodiscard]] LatestStockQuote get_latest_stock_quote(std::string const& symbol) const;
    [[nodiscard]] LatestOptionTrade get_latest_option_trade(std::string const& symbol,
                                                            LatestOptionTradeRequest const& request = {}) const;
    [[nodiscard]] LatestOptionQuote get_latest_option_quote(std::string const& symbol,
                                                            LatestOptionQuoteRequest const& request = {}) const;
    [[nodiscard]] LatestStockTrades get_latest_stock_trades(LatestStocksRequest const& request) const;
    [[nodiscard]] LatestStockQuotes get_latest_stock_quotes(LatestStocksRequest const& request) const;
    [[nodiscard]] LatestStockBars get_latest_stock_bars(LatestStocksRequest const& request) const;
    [[nodiscard]] LatestOptionTrades get_latest_option_trades(LatestOptionsRequest const& request) const;
    [[nodiscard]] LatestOptionQuotes get_latest_option_quotes(LatestOptionsRequest const& request) const;
    [[nodiscard]] LatestOptionBars get_latest_option_bars(LatestOptionsRequest const& request) const;
    [[nodiscard]] LatestCryptoTrades get_latest_crypto_trades(std::string const& feed,
                                                              LatestCryptoRequest const& request) const;
    [[nodiscard]] LatestCryptoQuotes get_latest_crypto_quotes(std::string const& feed,
                                                              LatestCryptoRequest const& request) const;
    [[nodiscard]] LatestCryptoBars get_latest_crypto_bars(std::string const& feed,
                                                          LatestCryptoRequest const& request) const;
    [[nodiscard]] MultiStockOrderbooks get_stock_orderbooks(LatestStockOrderbooksRequest const& request) const;
    [[nodiscard]] MultiOptionOrderbooks get_option_orderbooks(LatestOptionOrderbooksRequest const& request) const;
    [[nodiscard]] MultiCryptoOrderbooks get_crypto_orderbooks(std::string const& feed,
                                                              LatestCryptoOrderbooksRequest const& request) const;

    [[nodiscard]] StockBars get_stock_bars(std::string const& symbol, StockBarsRequest const& request = {}) const;
    [[nodiscard]] std::vector<StockBar> get_all_stock_bars(std::string const& symbol,
                                                           StockBarsRequest request = {}) const;
    [[nodiscard]] StockSnapshot get_stock_snapshot(std::string const& symbol) const;
    [[nodiscard]] MultiStockSnapshots get_stock_snapshots(MultiStockSnapshotsRequest const& request) const;
    [[nodiscard]] CryptoSnapshot get_crypto_snapshot(std::string const& feed, std::string const& symbol,
                                                     CryptoSnapshotRequest const& request = {}) const;
    [[nodiscard]] MultiCryptoSnapshots get_crypto_snapshots(std::string const& feed,
                                                            MultiCryptoSnapshotsRequest const& request) const;

    [[nodiscard]] PaginatedVectorRange<StockBarsRequest, StockBars, StockBar>
    stock_bars_range(std::string const& symbol, StockBarsRequest request = {}) const;

    [[nodiscard]] NewsResponse get_news(NewsRequest const& request = {}) const;
    [[nodiscard]] PaginatedVectorRange<NewsRequest, NewsResponse, NewsArticle>
    news_range(NewsRequest request = {}) const;

    /// Retrieves historical auctions for a single stock symbol.
    [[nodiscard]] HistoricalAuctionsResponse get_stock_auctions(std::string const& symbol,
                                                                HistoricalAuctionsRequest const& request = {}) const;

    /// Retrieves historical auctions across multiple symbols.
    [[nodiscard]] HistoricalAuctionsResponse get_auctions(HistoricalAuctionsRequest const& request) const;

    /// Streams historical auctions for a single symbol across all pages.
    [[nodiscard]] PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>
    stock_auctions_range(std::string const& symbol, HistoricalAuctionsRequest request = {}) const;

    /// Streams historical auctions across all requested symbols.
    [[nodiscard]] PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>
    auctions_range(HistoricalAuctionsRequest request = {}) const;
    [[nodiscard]] CorporateActionAnnouncementsResponse
    get_corporate_announcements(CorporateActionAnnouncementsRequest const& request = {}) const;
    [[nodiscard]] CorporateActionEventsResponse
    get_corporate_actions(CorporateActionEventsRequest const& request = {}) const;

    [[nodiscard]] MultiStockBars get_stock_aggregates(MultiStockBarsRequest const& request) const;
    [[nodiscard]] MultiStockQuotes get_stock_quotes(MultiStockQuotesRequest const& request) const;
    [[nodiscard]] MultiStockTrades get_stock_trades(MultiStockTradesRequest const& request) const;

    [[nodiscard]] MultiOptionBars get_option_aggregates(MultiOptionBarsRequest const& request) const;
    [[nodiscard]] MultiOptionQuotes get_option_quotes(MultiOptionQuotesRequest const& request) const;
    [[nodiscard]] MultiOptionTrades get_option_trades(MultiOptionTradesRequest const& request) const;
    [[nodiscard]] OptionSnapshot get_option_snapshot(std::string const& symbol,
                                                     OptionSnapshotRequest const& request = {}) const;
    [[nodiscard]] MultiOptionSnapshots get_option_snapshots(MultiOptionSnapshotsRequest const& request) const;
    [[nodiscard]] OptionChain get_option_chain(std::string const& symbol, OptionChainRequest const& request = {}) const;

    [[nodiscard]] MultiCryptoBars get_crypto_aggregates(MultiCryptoBarsRequest const& request) const;
    [[nodiscard]] MultiCryptoQuotes get_crypto_quotes(MultiCryptoQuotesRequest const& request) const;
    [[nodiscard]] MultiCryptoTrades get_crypto_trades(MultiCryptoTradesRequest const& request) const;
    [[nodiscard]] LatestCryptoTrades get_latest_crypto_trade(std::string const& feed,
                                                             LatestCryptoDataRequest const& request = {}) const;
    [[nodiscard]] LatestCryptoQuotes get_latest_crypto_quote(std::string const& feed,
                                                             LatestCryptoDataRequest const& request = {}) const;
    [[nodiscard]] LatestCryptoBars get_latest_crypto_bar(std::string const& feed,
                                                         LatestCryptoDataRequest const& request = {}) const;
    [[nodiscard]] LatestCryptoOrderbooks
    get_latest_crypto_orderbook(std::string const& feed, LatestCryptoOrderbookRequest const& request = {}) const;
    [[nodiscard]] ListExchangesResponse list_exchanges(ListExchangesRequest const& request) const;
    [[nodiscard]] ListTradeConditionsResponse list_trade_conditions(ListTradeConditionsRequest const& request) const;
    [[nodiscard]] MarketMoversResponse get_top_market_movers(MarketMoversRequest const& request) const;
    [[nodiscard]] MostActiveStocksResponse get_most_active_stocks(MostActiveStocksRequest const& request) const;

  private:
    RestClient v2_client_;
    RestClient beta_client_;
    RestClient beta_v3_client_;
    MarketDataPlan stock_data_plan_{MarketDataPlan::Auto};
    std::string stock_data_feed_{};
};

} // namespace alpaca
