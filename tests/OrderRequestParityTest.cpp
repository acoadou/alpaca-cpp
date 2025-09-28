#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/models/Common.hpp"
#include "alpaca/models/Order.hpp"

namespace {

using namespace std::chrono_literals;

TEST(NewOrderRequestParityTest, MarketOrderSerializationMatchesCSharpBehavior) {
    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "10";

    alpaca::Json json;
    to_json(json, request);

    alpaca::Json const expected = {
        {"symbol",        "AAPL"  },
        {"side",          "buy"   },
        {"type",          "market"},
        {"time_in_force", "day"   },
        {"qty",           "10"    }
    };

    EXPECT_EQ(json, expected);
}

TEST(NewOrderRequestParityTest, StopLimitOrderSerializationMatchesCSharpBehavior) {
    alpaca::NewOrderRequest request;
    request.symbol = "MSFT";
    request.side = alpaca::OrderSide::SELL;
    request.type = alpaca::OrderType::STOP_LIMIT;
    request.time_in_force = alpaca::TimeInForce::GTC;
    request.quantity = "25";
    request.limit_price = "200";
    request.stop_price = "190";
    request.client_order_id = "custom-client-id";
    request.high_water_mark = "250";

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json.at("symbol"), "MSFT");
    EXPECT_EQ(json.at("side"), "sell");
    EXPECT_EQ(json.at("type"), "stop_limit");
    EXPECT_EQ(json.at("time_in_force"), "gtc");
    EXPECT_EQ(json.at("qty"), "25");
    EXPECT_EQ(json.at("limit_price"), "200");
    EXPECT_EQ(json.at("stop_price"), "190");
    EXPECT_EQ(json.at("client_order_id"), "custom-client-id");
    EXPECT_EQ(json.at("high_water_mark"), "250");

    EXPECT_FALSE(json.contains("trail_price"));
    EXPECT_FALSE(json.contains("trail_percent"));
}

TEST(NewOrderRequestParityTest, TrailingStopOrderSerializationPrefersPercent) {
    alpaca::NewOrderRequest request;
    request.symbol = "TSLA";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::TRAILING_STOP;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "5";
    request.trail_percent = "4.2";

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json.at("type"), "trailing_stop");
    EXPECT_EQ(json.at("trail_percent"), "4.2");
    EXPECT_FALSE(json.contains("trail_price"));
}

TEST(NewOrderRequestParityTest, BracketOrderSerializationMatchesCSharpBehavior) {
    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "10";
    request.order_class = alpaca::OrderClass::BRACKET;

    alpaca::TakeProfitParams take_profit{.limit_price = "130"};
    alpaca::StopLossParams stop_loss;
    stop_loss.stop_price = "110";
    stop_loss.limit_price = "115";

    request.take_profit = take_profit;
    request.stop_loss = stop_loss;

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json.at("order_class"), "bracket");
    ASSERT_TRUE(json.contains("take_profit"));
    ASSERT_TRUE(json.at("take_profit").is_object());
    EXPECT_EQ(json.at("take_profit").at("limit_price"), "130");

    ASSERT_TRUE(json.contains("stop_loss"));
    ASSERT_TRUE(json.at("stop_loss").is_object());
    EXPECT_EQ(json.at("stop_loss").at("stop_price"), "110");
    EXPECT_EQ(json.at("stop_loss").at("limit_price"), "115");
}

TEST(NewOrderRequestParityTest, MultiLegOrderSerializationIncludesLegsAndIntent) {
    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "1";
    request.position_intent = alpaca::PositionIntent::AUTOMATIC;

    alpaca::OptionLeg first_leg;
    first_leg.symbol = "AAPL240621C00150000";
    first_leg.ratio = 2;
    first_leg.side = alpaca::OrderSide::SELL;
    first_leg.intent = alpaca::PositionIntent::CLOSING;

    alpaca::OptionLeg second_leg;
    second_leg.symbol = "AAPL240621P00150000";
    second_leg.ratio = 1;
    second_leg.side = alpaca::OrderSide::BUY;
    second_leg.intent = alpaca::PositionIntent::OPENING;

    request.legs = {first_leg, second_leg};

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json.at("position_intent"), "automatic");
    ASSERT_TRUE(json.contains("legs"));
    ASSERT_TRUE(json.at("legs").is_array());
    ASSERT_EQ(json.at("legs").size(), 2);

    auto const& first_leg_json = json.at("legs")[0];
    EXPECT_EQ(first_leg_json.at("symbol"), "AAPL240621C00150000");
    EXPECT_EQ(first_leg_json.at("ratio"), 2);
    EXPECT_EQ(first_leg_json.at("side"), "sell");
    EXPECT_EQ(first_leg_json.at("position_intent"), "closing");

    auto const& second_leg_json = json.at("legs")[1];
    EXPECT_EQ(second_leg_json.at("symbol"), "AAPL240621P00150000");
    EXPECT_EQ(second_leg_json.at("ratio"), 1);
    EXPECT_EQ(second_leg_json.at("side"), "buy");
    EXPECT_EQ(second_leg_json.at("position_intent"), "opening");
}

TEST(ReplaceOrderRequestParityTest, SerializationMatchesCSharpBehavior) {
    alpaca::ReplaceOrderRequest request;
    request.quantity = "42";
    request.limit_price = "210";
    request.stop_price = "205";
    request.time_in_force = "gtc";
    request.extended_hours = true;
    request.client_order_id = "replace-client-id";

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json.at("qty"), "42");
    EXPECT_EQ(json.at("limit_price"), "210");
    EXPECT_EQ(json.at("stop_price"), "205");
    EXPECT_EQ(json.at("time_in_force"), "gtc");
    EXPECT_TRUE(json.at("extended_hours").get<bool>());
    EXPECT_EQ(json.at("client_order_id"), "replace-client-id");
}

TEST(ListOrdersRequestParityTest, QueryParamsMatchCSharpBehavior) {
    using namespace std::chrono;

    alpaca::ListOrdersRequest request;
    request.status = alpaca::OrderStatusFilter::CLOSED;
    request.limit = 50;
    request.after = sys_days{2023y / 3 / 7} + 9h + 30min;
    request.until = sys_days{2023y / 3 / 8} + 16h;
    request.direction = alpaca::SortDirection::DESC;
    request.nested = true;
    request.symbols = {"AAPL", "MSFT"};
    request.asset_class = alpaca::AssetClass::US_EQUITY;
    request.venue = "PSE";

    auto const params = request.to_query_params();

    ASSERT_EQ(params.size(), 9U);
    EXPECT_EQ(params[0].first, "status");
    EXPECT_EQ(params[0].second, "closed");
    EXPECT_EQ(params[1].first, "limit");
    EXPECT_EQ(params[1].second, "50");
    EXPECT_EQ(params[2].first, "after");
    EXPECT_EQ(params[2].second, alpaca::format_timestamp(*request.after));
    EXPECT_EQ(params[3].first, "until");
    EXPECT_EQ(params[3].second, alpaca::format_timestamp(*request.until));
    EXPECT_EQ(params[4].first, "direction");
    EXPECT_EQ(params[4].second, "desc");
    EXPECT_EQ(params[5].first, "nested");
    EXPECT_EQ(params[5].second, "true");
    EXPECT_EQ(params[6].first, "symbols");
    EXPECT_EQ(params[6].second, "AAPL,MSFT");
    EXPECT_EQ(params[7].first, "asset_class");
    EXPECT_EQ(params[7].second, "us_equity");
    EXPECT_EQ(params[8].first, "venue");
    EXPECT_EQ(params[8].second, "PSE");
}

} // namespace
