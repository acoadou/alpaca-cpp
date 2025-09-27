#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include <chrono>

#include "alpaca/AlpacaClient.hpp"
#include "alpaca/Json.hpp"

namespace {

class StubHttpClient : public alpaca::HttpClient {
  public:
    StubHttpClient() = default;

    void enqueue_response(alpaca::HttpResponse response) {
        responses_.push(std::move(response));
    }

    [[nodiscard]] std::vector<alpaca::HttpRequest> const& requests() const {
        return requests_;
    }

    alpaca::HttpResponse send(alpaca::HttpRequest const& request) override {
        requests_.push_back(request);
        if (responses_.empty()) {
            return alpaca::HttpResponse{};
        }
        auto response = std::move(responses_.front());
        responses_.pop();
        return response;
    }

  private:
    std::queue<alpaca::HttpResponse> responses_{};
    std::vector<alpaca::HttpRequest> requests_{};
};

TEST(ConfigurationTest, FromEnvironmentIncludesStreamingEndpoints) {
    auto environment = alpaca::Environments::Paper();
    alpaca::Configuration cfg = alpaca::Configuration::FromEnvironment(environment, "id", "secret");

    EXPECT_EQ(cfg.trading_base_url, environment.trading_base_url);
    EXPECT_EQ(cfg.data_base_url, environment.data_base_url);
    EXPECT_EQ(cfg.trading_stream_url, environment.trading_stream_url);
    EXPECT_EQ(cfg.market_data_stream_url, environment.market_data_stream_url);
    EXPECT_EQ(cfg.crypto_stream_url, environment.crypto_stream_url);
}

TEST(AlpacaClientTest, SubmitOrderSerializesAdvancedFields) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"id":"order-1","symbol":"AAPL","side":"buy","type":"limit","created_at":"2023-01-01T00:00:00Z",
           "time_in_force":"day","status":"accepted"})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::LIMIT;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "10";
    request.limit_price = "125";
    request.order_class = alpaca::OrderClass::BRACKET;
    request.take_profit = alpaca::TakeProfitParams{"140"};
    request.stop_loss = alpaca::StopLossParams{std::make_optional<std::string>("110"), std::nullopt};

    alpaca::Order const order = client.submit_order(request);
    EXPECT_EQ(order.id, "order-1");
    EXPECT_EQ(order.symbol, "AAPL");
    EXPECT_EQ(order.side, alpaca::OrderSide::BUY);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(http_request.url, config.trading_base_url + "/v2/orders");

    alpaca::Json const json = alpaca::Json::parse(http_request.body);
    EXPECT_EQ(json.at("order_class"), "bracket");
    EXPECT_EQ(json.at("take_profit").at("limit_price"), "140");
    EXPECT_EQ(json.at("stop_loss").at("stop_price"), "110");
}

TEST(AlpacaClientTest, NewsRangeRespectsRetryAfterAndPagination) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{429, R"({"message":"rate limit"})", {{"Retry-After", "0"}}});
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"news":[{"id":"n1","headline":"First","url":"https://example.com/1","source":"X","symbols":["AAPL"]}],"next_page_token":"cursor"})",
        {}});
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"news":[{"id":"n2","headline":"Second","url":"https://example.com/2","source":"X","symbols":["AAPL"]}],"next_page_token":null})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::NewsRequest request;
    request.symbols = {"AAPL"};
    request.limit = 1;

    std::vector<std::string> headlines;
    for (auto const& article : client.news_range(request)) {
        headlines.push_back(article.headline);
    }

    ASSERT_EQ(headlines.size(), 2U);
    EXPECT_EQ(headlines.front(), "First");
    EXPECT_EQ(headlines.back(), "Second");

    EXPECT_EQ(stub->requests().size(), 3U);
    EXPECT_NE(stub->requests()[1].url.find("symbols=AAPL"), std::string::npos);
    EXPECT_NE(stub->requests()[2].url.find("page_token=cursor"), std::string::npos);
}

TEST(AlpacaClientTest, CalendarRequestsIncludeQueryParameters) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200, R"([{"date":"2023-01-03","open":"09:30","close":"16:00"}])", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::CalendarRequest request;
    request.start = std::chrono::sys_days{std::chrono::year{2023} / std::chrono::January / 3};

    auto const calendar = client.get_calendar(request);
    ASSERT_EQ(calendar.size(), 1U);
    EXPECT_EQ(calendar.front().date, "2023-01-03");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_NE(http_request.url.find("start=2023-01-03"), std::string::npos);
}

TEST(AlpacaClientTest, LatestStockTradeUsesMarketDataEndpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200, R"({"symbol":"AAPL","trade":{"i":"t1","x":"P","p":123.45,"s":10,"t":"2023-01-01T00:00:00Z"}})", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    auto const latest = client.get_latest_stock_trade("AAPL");
    EXPECT_EQ(latest.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(latest.trade.price, 123.45);
    EXPECT_EQ(latest.trade.size, 10U);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(http_request.url, config.data_base_url + "/stocks/AAPL/trades/latest");
}

TEST(AlpacaClientTest, StockBarsForwardQueryParameters) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200, R"({"symbol":"AAPL","bars":[{"t":"2023-01-01T00:00:00Z","o":1.0,"h":1.5,"l":0.5,"c":1.2,"v":100}]})", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::StockBarsRequest request;
    request.timeframe = "1Min";
    request.limit = 1;

    auto const bars = client.get_stock_bars("AAPL", request);
    EXPECT_EQ(bars.symbol, "AAPL");
    ASSERT_EQ(bars.bars.size(), 1U);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_NE(http_request.url.find("timeframe=1Min"), std::string::npos);
    EXPECT_NE(http_request.url.find("limit=1"), std::string::npos);
    EXPECT_EQ(http_request.url.substr(0, config.data_base_url.size()), config.data_base_url);
}

TEST(AlpacaClientTest, GetAllStockBarsTraversesPagination) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"symbol":"AAPL","bars":[{"t":"2023-01-01T00:00:00Z","o":1.0,"h":1.2,"l":0.9,"c":1.1,"v":100}],"next_page_token":"token"})",
        {}});
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"symbol":"AAPL","bars":[{"t":"2023-01-01T00:01:00Z","o":1.1,"h":1.3,"l":1.0,"c":1.2,"v":80}],"next_page_token":null})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::StockBarsRequest request;
    request.timeframe = "1Min";

    auto const bars = client.get_all_stock_bars("AAPL", request);
    ASSERT_EQ(bars.size(), 2U);
    EXPECT_EQ(bars.front().timestamp, alpaca::parse_timestamp("2023-01-01T00:00:00Z"));
    EXPECT_EQ(bars.back().timestamp, alpaca::parse_timestamp("2023-01-01T00:01:00Z"));

    ASSERT_EQ(stub->requests().size(), 2U);
    EXPECT_NE(stub->requests()[0].url.find("timeframe=1Min"), std::string::npos);
    EXPECT_NE(stub->requests()[1].url.find("page_token=token"), std::string::npos);
}

TEST(AlpacaClientTest, DeleteBrokerAccountUsesCloseActionEndpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{204, {}, {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    client.delete_broker_account("account-1");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& request = stub->requests().front();
    EXPECT_EQ(request.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(request.url, config.broker_base_url + "/v1/accounts/account-1/actions/close");
}

TEST(AlpacaClientTest, CancelTransferUsesAccountScopedDeleteEndpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{204, {}, {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    client.cancel_transfer("account-1", "transfer-1");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& request = stub->requests().front();
    EXPECT_EQ(request.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(request.url, config.broker_base_url + "/v1/accounts/account-1/transfers/transfer-1");
}

TEST(AlpacaClientTest, CancelJournalUsesDeleteEndpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{204, {}, {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    client.cancel_journal("journal-1");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& request = stub->requests().front();
    EXPECT_EQ(request.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(request.url, config.broker_base_url + "/v1/journals/journal-1");
}

} // namespace
