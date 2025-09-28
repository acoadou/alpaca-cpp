#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "FakeHttpClient.hpp"
#include "alpaca/Configuration.hpp"
#include "alpaca/Exceptions.hpp"
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
        {"AAPL240119C00195000", 1, alpaca::OrderSide::BUY,  alpaca::PositionIntent::OPENING},
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

TEST(TradingClientTest, SubmitOrderSerializesAdvancedFields) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"order-advanced","asset_id":"","client_order_id":"client-123","account_id":"account","created_at":"2023-01-01T00:00:00Z","replaced_by":"","replaces":"","symbol":"AAPL","asset_class":"us_equity","side":"buy","type":"market","time_in_force":"day","status":"accepted"})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.notional = "1000";
    request.client_order_id = "client-advanced";
    request.order_class = alpaca::OrderClass::BRACKET;
    request.take_profit = alpaca::TakeProfitParams{"105"};
    request.stop_loss = alpaca::StopLossParams{std::string{"95"}, std::string{"94"}};

    auto const order = client.submit_order(request);
    EXPECT_EQ(order.id, "order-advanced");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/orders");

    auto const payload = alpaca::Json::parse(recorded.body);
    EXPECT_EQ(payload.at("notional"), "1000");
    EXPECT_EQ(payload.at("client_order_id"), "client-advanced");
    EXPECT_EQ(payload.at("order_class"), "bracket");
    ASSERT_TRUE(payload.contains("take_profit"));
    EXPECT_EQ(payload.at("take_profit").at("limit_price"), "105");
    ASSERT_TRUE(payload.contains("stop_loss"));
    EXPECT_EQ(payload.at("stop_loss").at("stop_price"), "95");
    EXPECT_EQ(payload.at("stop_loss").at("limit_price"), "94");
}

TEST(TradingClientTest, ReplaceOrderSerializesClientOrderId) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"order-replaced","asset_id":"","client_order_id":"client-replaced","account_id":"account","created_at":"2023-01-01T00:00:00Z","replaced_by":"","replaces":"","symbol":"AAPL","asset_class":"us_equity","side":"buy","type":"limit","time_in_force":"day","status":"accepted"})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::ReplaceOrderRequest request;
    request.limit_price = "101";
    request.client_order_id = "client-replace";

    auto const order = client.replace_order("order-advanced", request);
    EXPECT_EQ(order.id, "order-replaced");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::PATCH);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/orders/order-advanced");

    auto const payload = alpaca::Json::parse(recorded.body);
    EXPECT_EQ(payload.at("limit_price"), "101");
    EXPECT_EQ(payload.at("client_order_id"), "client-replace");
}

TEST(TradingClientTest, SubmitCryptoOrderIncludesRoutingControls) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"crypto-order","asset_id":"","client_order_id":"","account_id":"account","created_at":"2023-01-01T00:00:00Z","replaced_by":"","replaces":"","symbol":"BTCUSD","asset_class":"crypto","side":"buy","type":"market","time_in_force":"gtc","status":"accepted"})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::NewCryptoOrderRequest request;
    request.symbol = "BTCUSD";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::GTC;
    request.quantity = "0.1";
    request.quote_symbol = "USD";
    request.venue = "CBSE";
    request.reduce_only = true;

    auto const order = client.submit_crypto_order(request);
    EXPECT_EQ(order.id, "crypto-order");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/crypto/orders");

    auto const payload = alpaca::Json::parse(recorded.body);
    EXPECT_EQ(payload.at("quote_symbol"), "USD");
    EXPECT_EQ(payload.at("venue"), "CBSE");
    EXPECT_TRUE(payload.at("reduce_only").get<bool>());
}

TEST(TradingClientTest, SubmitOtcOrderIncludesRoutingControls) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"otc-order","asset_id":"","client_order_id":"","account_id":"account","created_at":"2023-01-01T00:00:00Z","replaced_by":"","replaces":"","symbol":"EURUSD","asset_class":"forex","side":"buy","type":"market","time_in_force":"gtc","status":"accepted"})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::NewOtcOrderRequest request;
    request.symbol = "EURUSD";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::GTC;
    request.notional = "10000";
    request.quote_symbol = "USD";
    request.venue = "OTC";
    request.reduce_only = true;

    auto const order = client.submit_otc_order(request);
    EXPECT_EQ(order.id, "otc-order");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/otc/orders");

    auto const payload = alpaca::Json::parse(recorded.body);
    EXPECT_EQ(payload.at("quote_symbol"), "USD");
    EXPECT_EQ(payload.at("venue"), "OTC");
    EXPECT_TRUE(payload.at("reduce_only").get<bool>());
}

TEST(TradingClientTest, CustomRetryOptionsPropagateToRestClient) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{500, R"({"message":"fail"})", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::RestClient::Options options;
    options.retry.max_attempts = 1;
    options.retry.retry_status_codes = {500};
    options.retry.initial_backoff = std::chrono::milliseconds{0};
    options.retry.max_backoff = std::chrono::milliseconds{0};

    alpaca::TradingClient client(config, http, options);

    EXPECT_THROW(client.get_account(), alpaca::Exception);
    ASSERT_EQ(http->requests().size(), 1U);
}

} // namespace
