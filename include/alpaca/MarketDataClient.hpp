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
    MarketDataClient(Configuration const& config, HttpClientPtr http_client = nullptr);
    MarketDataClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                     HttpClientPtr http_client = nullptr);

    [[nodiscard]] LatestStockTrade get_latest_stock_trade(std::string const& symbol) const;
    [[nodiscard]] LatestStockQuote get_latest_stock_quote(std::string const& symbol) const;

    [[nodiscard]] StockBars get_stock_bars(std::string const& symbol, StockBarsRequest const& request = {}) const;
    [[nodiscard]] std::vector<StockBar> get_all_stock_bars(std::string const& symbol,
                                                           StockBarsRequest request = {}) const;
    [[nodiscard]] StockSnapshot get_stock_snapshot(std::string const& symbol) const;

    [[nodiscard]] PaginatedVectorRange<StockBarsRequest, StockBars, StockBar>
    stock_bars_range(std::string const& symbol, StockBarsRequest request = {}) const;

    [[nodiscard]] NewsResponse get_news(NewsRequest const& request = {}) const;
    [[nodiscard]] PaginatedVectorRange<NewsRequest, NewsResponse, NewsArticle>
    news_range(NewsRequest request = {}) const;
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

    [[nodiscard]] MultiCryptoBars get_crypto_aggregates(MultiCryptoBarsRequest const& request) const;
    [[nodiscard]] MultiCryptoQuotes get_crypto_quotes(MultiCryptoQuotesRequest const& request) const;
    [[nodiscard]] MultiCryptoTrades get_crypto_trades(MultiCryptoTradesRequest const& request) const;

  private:
    RestClient v2_client_;
    RestClient beta_client_;
};

} // namespace alpaca
