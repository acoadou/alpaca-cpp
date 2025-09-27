#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "alpaca/BrokerClient.hpp"
#include "alpaca/Json.hpp"

namespace {

class StubHttpClient : public alpaca::HttpClient {
  public:
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

} // namespace

TEST(BrokerClientModelTest, RebalancingPortfolioParsesWeightsAndConditions) {
    auto json = alpaca::Json::parse(R"JSON(
        {
          "id": "port-1",
          "name": "Growth",
          "description": "Aggressive growth portfolio",
          "status": "active",
          "cooldown_days": 5,
          "created_at": "2024-01-01T00:00:00Z",
          "updated_at": "2024-01-05T00:00:00Z",
          "weights": [
            {"type": "asset", "symbol": "AAPL", "percent": 55.25},
            {"type": "asset", "symbol": "MSFT", "percent": 44.75}
          ],
          "rebalance_conditions": [
            {"type": "drift", "sub_type": "percent", "percent": 5.0},
            {"type": "calendar", "sub_type": "weekly", "day": "monday"}
          ]
        }
    )JSON");

    alpaca::RebalancingPortfolio const portfolio = json.get<alpaca::RebalancingPortfolio>();
    EXPECT_EQ(portfolio.id, "port-1");
    ASSERT_EQ(portfolio.weights.size(), 2U);
    EXPECT_DOUBLE_EQ(portfolio.weights[0].percent, 55.25);
    ASSERT_TRUE(portfolio.rebalance_conditions.has_value());
    EXPECT_EQ(portfolio.rebalance_conditions->at(1).day, "monday");
}

TEST(BrokerClientModelTest, ManagedPortfolioHistoryParsesCashflowsAndNulls) {
    auto json = alpaca::Json::parse(R"JSON(
        {
          "timestamp": [1704067200, 1704153600],
          "equity": [100000.0, 101500.5],
          "profit_loss": [0.0, 1500.5],
          "profit_loss_pct": [null, 0.015],
          "base_value": 100000.0,
          "timeframe": "1D",
          "cashflow": {
            "dividend": [0.0, 10.0]
          }
        }
    )JSON");

    alpaca::ManagedPortfolioHistory const history = json.get<alpaca::ManagedPortfolioHistory>();
    ASSERT_EQ(history.timestamp.size(), 2U);
    ASSERT_EQ(history.profit_loss_pct.size(), 2U);
    EXPECT_FALSE(history.profit_loss_pct[0].has_value());
    EXPECT_TRUE(history.profit_loss_pct[1].has_value());
    ASSERT_EQ(history.cashflow.count("dividend"), 1U);
    EXPECT_DOUBLE_EQ(history.cashflow.at("dividend")[1], 10.0);
}

TEST(BrokerClientTest, CreateBrokerWatchlistSerializesPayload) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(
    alpaca::HttpResponse{200,
                         R"JSON({"id":"wl-1","name":"Focus","account_id":"acct-1","created_at":"2024-01-01T00:00:00Z",
                "updated_at":"2024-01-01T00:00:00Z","assets":[]})JSON",
                         {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::BrokerClient client(config, stub);

    alpaca::CreateBrokerWatchlistRequest request{
        "Focus", {"AAPL", "TSLA"}
    };
    alpaca::BrokerWatchlist const watchlist = client.create_watchlist("acct-1", request);
    EXPECT_EQ(watchlist.id, "wl-1");

    ASSERT_EQ(stub->requests().size(), 1U);
    alpaca::HttpRequest const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(http_request.url, config.broker_base_url + "/trading/accounts/acct-1/watchlists");

    alpaca::Json const body = alpaca::Json::parse(http_request.body);
    ASSERT_TRUE(body.contains("symbols"));
    EXPECT_EQ(body.at("symbols").size(), 2);
    EXPECT_EQ(body.at("symbols")[0], "AAPL");
}

TEST(BrokerClientTest, ListRebalancingPortfoliosHandlesWrappedResponse) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(
    alpaca::HttpResponse{200,
                         R"JSON({"portfolios":[{"id":"p1","name":"Core","description":"","status":"active",
                "cooldown_days":0,"created_at":"2024-01-01T00:00:00Z","updated_at":"2024-01-01T00:00:00Z",
                "weights":[],"rebalance_conditions":[]}]})JSON",
                         {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::BrokerClient client(config, stub);

    std::vector<alpaca::RebalancingPortfolio> const portfolios = client.list_rebalancing_portfolios();
    ASSERT_EQ(portfolios.size(), 1U);
    EXPECT_EQ(portfolios.front().id, "p1");

    ASSERT_EQ(stub->requests().size(), 1U);
    EXPECT_EQ(stub->requests().front().url, config.broker_base_url + "/rebalancing/portfolios");
}

TEST(BrokerClientTest, ListRebalancingSubscriptionsRangeIteratesPages) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(
    alpaca::HttpResponse{200,
                         R"JSON({"subscriptions":[{"id":"sub-1","account_id":"acct-1","portfolio_id":"p1",
                "created_at":"2024-01-01T00:00:00Z"},{"id":"sub-2","account_id":"acct-2","portfolio_id":"p1",
                "created_at":"2024-01-01T00:00:00Z"}],"next_page_token":"token-2"})JSON",
                         {}});
    stub->enqueue_response(
    alpaca::HttpResponse{200,
                         R"JSON({"subscriptions":[{"id":"sub-3","account_id":"acct-3","portfolio_id":"p1",
                "created_at":"2024-01-01T00:00:00Z"}]})JSON",
                         {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::BrokerClient client(config, stub);

    alpaca::ListRebalancingSubscriptionsRequest request;
    request.limit = 2;
    std::vector<std::string> ids;
    for (auto const& subscription : client.list_rebalancing_subscriptions_range(request)) {
        ids.push_back(subscription.id);
        if (ids.size() == 3U) {
            break;
        }
    }

    EXPECT_EQ(ids, (std::vector<std::string>{"sub-1", "sub-2", "sub-3"}));
    ASSERT_EQ(stub->requests().size(), 2U);
    EXPECT_EQ(stub->requests()[0].url, config.broker_base_url + "/rebalancing/subscriptions?limit=2");
    EXPECT_NE(stub->requests()[1].url.find("page_token=token-2"), std::string::npos);
}
