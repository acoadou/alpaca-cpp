#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>

#include "FakeHttpClient.hpp"
#include "alpaca/Configuration.hpp"
#include "alpaca/MarketDataClient.hpp"
#include "alpaca/models/MarketData.hpp"

namespace {

alpaca::HttpResponse MakeHttpResponse(std::string body) {
    return alpaca::HttpResponse{200, std::move(body), {}};
}

} // namespace

TEST(MarketDataClientTest, MultiLatestStockTradesSerializesSymbols) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({
        "trades": {
            "AAPL": {"i": "t1", "x": "P", "p": 150.25, "s": 100, "t": "2023-01-01T00:00:00Z", "c": ["@"], "z": "C"},
            "MSFT": {"i": "t2", "x": "Q", "p": 250.50, "s": 200, "t": "2023-01-01T00:00:01Z", "c": ["@"], "z": "C"}
        }
    })"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::LatestStocksRequest request;
    request.symbols = {"AAPL", "MSFT"};
    request.feed = "sip";

    alpaca::LatestStockTrades trades = client.get_latest_stock_trades(request);
    ASSERT_EQ(trades.trades.size(), 2U);
    EXPECT_DOUBLE_EQ(trades.trades.at("AAPL").price, 150.25);
    EXPECT_EQ(trades.trades.at("MSFT").exchange, "Q");

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v2/stocks/trades/latest") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("symbols=AAPL%2CMSFT") != std::string::npos ||
                http_request.url.find("symbols=AAPL,MSFT") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("feed=sip") != std::string::npos);
}

TEST(MarketDataClientTest, MultiLatestOptionQuotesTargetsBetaEndpoint) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({
        "quotes": {
            "AAPL240119C00195000": {
                "ax": "W", "ap": 5.5, "as": 10,
                "bx": "V", "bp": 5.4, "bs": 8,
                "t": "2023-01-01T00:00:00Z", "c": ["@"], "z": "C"
            }
        }
    })"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::LatestOptionsRequest request;
    request.symbols = {"AAPL240119C00195000"};
    request.feed = "opra";

    alpaca::LatestOptionQuotes quotes = client.get_latest_option_quotes(request);
    ASSERT_EQ(quotes.quotes.size(), 1U);
    EXPECT_EQ(quotes.quotes.at("AAPL240119C00195000").ask_exchange, "W");

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v1beta1/options/quotes/latest") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("symbols=AAPL240119C00195000") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("feed=opra") != std::string::npos);
}

TEST(MarketDataClientTest, CryptoOrderbooksParseSnapshots) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({
        "orderbooks": {
            "BTC/USD": {
                "t": "2023-01-01T00:00:00Z",
                "b": [{"p": 27000.0, "s": 0.5}],
                "a": [{"p": 27010.0, "s": 0.25}]
            }
        }
    })"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::LatestCryptoOrderbooksRequest request;
    request.symbols = {"BTC/USD"};

    alpaca::MultiCryptoOrderbooks orderbooks = client.get_crypto_orderbooks("US", request);
    ASSERT_EQ(orderbooks.orderbooks.size(), 1U);
    auto const& snapshot = orderbooks.orderbooks.at("BTC/USD");
    ASSERT_EQ(snapshot.bids.size(), 1U);
    EXPECT_DOUBLE_EQ(snapshot.bids.front().price, 27000.0);
    ASSERT_EQ(snapshot.asks.size(), 1U);
    EXPECT_DOUBLE_EQ(snapshot.asks.front().size, 0.25);

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v1beta1/crypto/us/latest/orderbooks") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("symbols=BTC%2FUSD") != std::string::npos ||
                http_request.url.find("symbols=BTC/USD") != std::string::npos);
}

TEST(MarketDataClientTest, GetLatestCryptoTradesThrowsOnEmptyFeed) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake = std::make_shared<FakeHttpClient>();
    alpaca::MarketDataClient client(config, fake);

    alpaca::LatestCryptoRequest request;
    request.symbols = {"BTC/USD"};

    EXPECT_THROW(client.get_latest_crypto_trades("", request), std::invalid_argument);
}

TEST(MarketDataClientTest, ListExchangesFormatsPath) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({"exchanges":[{"id":"NYSE","name":"New York Stock Exchange"}]})"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::ListExchangesRequest request;
    request.asset_class = "Stocks";
    request.locale = "us";

    alpaca::ListExchangesResponse response = client.list_exchanges(request);
    ASSERT_EQ(response.exchanges.size(), 1U);
    EXPECT_EQ(response.exchanges.front().id, "NYSE");

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v2/meta/exchanges/stocks") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("locale=us") != std::string::npos);
}

TEST(MarketDataClientTest, ListTradeConditionsIncludesParameters) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({"conditions":[{"id":"@","name":"Regular","type":"trade"}]})"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::ListTradeConditionsRequest request;
    request.asset_class = "Stocks";
    request.condition_type = "TRADES";
    request.sip = "CTA";

    alpaca::ListTradeConditionsResponse response = client.list_trade_conditions(request);
    ASSERT_EQ(response.conditions.size(), 1U);
    EXPECT_EQ(response.conditions.front().id, "@");
    EXPECT_EQ(response.conditions.front().type.value_or(""), "trade");

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v2/meta/conditions/stocks/trades") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("sip=CTA") != std::string::npos);
}

TEST(MarketDataClientTest, MarketMoversParsesPayload) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({
        "gainers": [{"symbol":"AAPL","percent_change":2.5,"change":4.0,"price":160.0}],
        "losers": [],
        "market_type": "stocks",
        "last_updated": "2023-01-01T00:00:00Z"
    })"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::MarketMoversRequest request;
    request.market_type = "STOCKS";
    request.top = 5;

    alpaca::MarketMoversResponse movers = client.get_top_market_movers(request);
    ASSERT_EQ(movers.gainers.size(), 1U);
    EXPECT_EQ(movers.gainers.front().symbol, "AAPL");
    EXPECT_EQ(movers.market_type, "stocks");

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v1beta1/screener/stocks/movers") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("top=5") != std::string::npos);
}

TEST(MarketDataClientTest, MostActiveStocksSerializesQuery) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(MakeHttpResponse(R"({
        "most_actives": [{"symbol":"AAPL","volume":1000.0,"trade_count":50.0}],
        "last_updated": "2023-01-01T00:00:00Z"
    })"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient client(config, fake);

    alpaca::MostActiveStocksRequest request;
    request.by = "volume";
    request.top = 3;

    alpaca::MostActiveStocksResponse response = client.get_most_active_stocks(request);
    ASSERT_EQ(response.most_actives.size(), 1U);
    EXPECT_EQ(response.most_actives.front().symbol, "AAPL");

    ASSERT_EQ(fake->requests().size(), 1U);
    auto const& http_request = fake->requests().front().request;
    EXPECT_TRUE(http_request.url.find("/v1beta1/screener/stocks/most-actives") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("by=volume") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("top=3") != std::string::npos);
}
