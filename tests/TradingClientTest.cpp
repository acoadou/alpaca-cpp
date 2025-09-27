#include <gtest/gtest.h>

#include <memory>

#include "FakeHttpClient.hpp"
#include "alpaca/Configuration.hpp"
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

} // namespace
