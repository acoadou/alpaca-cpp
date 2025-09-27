#include <gtest/gtest.h>

#include <memory>

#include "FakeHttpClient.hpp"
#include "alpaca/Configuration.hpp"
#include "alpaca/Json.hpp"
#include "alpaca/TradingClient.hpp"

namespace {

TEST(TradingClientTest, CloseAllPositionsTargetsPositionsEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{200, R"([])", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::CloseAllPositionsRequest request;
    request.cancel_orders = true;

    auto const responses = client.close_all_positions(request);
    EXPECT_TRUE(responses.empty());

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/positions?cancel_orders=true");
}

TEST(TradingClientTest, ExerciseOptionsPositionPostsToExerciseEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{204, std::string{}, {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    client.exercise_options_position("AAPL240119C00195000");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/positions/AAPL240119C00195000/exercise");
    EXPECT_EQ(recorded.body, "{}");
}

TEST(TradingClientTest, SubmitOptionOrderIncludesMultiLegPayload) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"order-123","client_order_id":"","account_id":"account","created_at":"2023-01-01T00:00:00Z","status":"accepted"})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::NewOptionOrderRequest request;
    request.symbol = "AAPL240119C00195000";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::LIMIT;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "1";
    request.limit_price = "2.50";
    request.position_intent = alpaca::PositionIntent::OPENING;
    request.legs = {
        {"AAPL240119C00195000", 1, alpaca::OrderSide::BUY, alpaca::PositionIntent::OPENING},
        {"AAPL240119P00195000", 1, alpaca::OrderSide::SELL, alpaca::PositionIntent::CLOSING}
    };

    auto const order = client.submit_option_order(request);
    EXPECT_EQ(order.id, "order-123");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/options/orders");

    auto const payload = alpaca::Json::parse(recorded.body);
    EXPECT_EQ(payload.at("symbol"), "AAPL240119C00195000");
    EXPECT_EQ(payload.at("position_intent"), "opening");
    ASSERT_TRUE(payload.contains("legs"));
    ASSERT_EQ(payload.at("legs").size(), 2);
    EXPECT_EQ(payload.at("legs")[0].at("symbol"), "AAPL240119C00195000");
    EXPECT_EQ(payload.at("legs")[1].at("position_intent"), "closing");
}

} // namespace
