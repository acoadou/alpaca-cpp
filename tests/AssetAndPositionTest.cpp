#include <gtest/gtest.h>

#include "alpaca/models/Asset.hpp"
#include "alpaca/models/Clock.hpp"
#include "alpaca/models/Position.hpp"

namespace {

TEST(AssetModelTest, FromJsonParsesOptionalFields) {
  const alpaca::Json json = {{"id", "asset-id"},
                             {"symbol", "AAPL"},
                             {"name", "Apple"},
                             {"exchange", "NASDAQ"},
                             {"class", "CRYPTO"},
                             {"status", "INACTIVE"},
                             {"tradable", true},
                             {"marginable", true},
                             {"shortable", false},
                             {"easy_to_borrow", true},
                             {"fractionable", true},
                             {"maintenance_margin_requirement", "25"}};

  const auto asset = json.get<alpaca::Asset>();

  EXPECT_EQ(asset.id, "asset-id");
  EXPECT_EQ(asset.symbol, "AAPL");
  EXPECT_EQ(asset.name, "Apple");
  EXPECT_EQ(asset.exchange, "NASDAQ");
  EXPECT_EQ(asset.asset_class, alpaca::AssetClass::CRYPTO);
  EXPECT_EQ(asset.status, alpaca::AssetStatus::INACTIVE);
  EXPECT_TRUE(asset.tradable);
  EXPECT_TRUE(asset.marginable);
  EXPECT_FALSE(asset.shortable);
  EXPECT_TRUE(asset.easy_to_borrow);
  EXPECT_TRUE(asset.fractionable);
  EXPECT_EQ(asset.maintenance_margin_requirement, "25");
}

TEST(AssetModelTest, FromJsonUsesDefaultsWhenOptionalFieldsMissing) {
  const alpaca::Json json = {{"id", "asset-id"}};

  const auto asset = json.get<alpaca::Asset>();

  EXPECT_EQ(asset.symbol, "");
  EXPECT_EQ(asset.name, "");
  EXPECT_EQ(asset.exchange, "");
  EXPECT_EQ(asset.asset_class, alpaca::AssetClass::US_EQUITY);
  EXPECT_EQ(asset.status, alpaca::AssetStatus::ACTIVE);
  EXPECT_FALSE(asset.tradable);
  EXPECT_FALSE(asset.marginable);
  EXPECT_FALSE(asset.shortable);
  EXPECT_FALSE(asset.easy_to_borrow);
  EXPECT_FALSE(asset.fractionable);
  EXPECT_EQ(asset.maintenance_margin_requirement, "");
}

TEST(AssetModelTest, ListAssetsRequestBuildsQueryParams) {
  alpaca::ListAssetsRequest request;
  request.status = alpaca::AssetStatus::INACTIVE;
  request.asset_class = alpaca::AssetClass::CRYPTO;
  request.exchange = "NYSE";
  request.symbols = {"AAPL", "MSFT"};

  const auto params = request.to_query_params();

  ASSERT_EQ(params.size(), 4U);
  EXPECT_EQ(params[0].first, "status");
  EXPECT_EQ(params[0].second, "inactive");
  EXPECT_EQ(params[1].first, "asset_class");
  EXPECT_EQ(params[1].second, "crypto");
  EXPECT_EQ(params[2].first, "exchange");
  EXPECT_EQ(params[2].second, "NYSE");
  EXPECT_EQ(params[3].first, "symbols");
  EXPECT_EQ(params[3].second, "AAPL,MSFT");
}

TEST(AssetModelTest, ListAssetsRequestSkipsUnsetFields) {
  const alpaca::ListAssetsRequest request;

  const auto params = request.to_query_params();

  EXPECT_TRUE(params.empty());
}

TEST(PositionModelTest, FromJsonParsesAllFields) {
  const alpaca::Json json = {{"asset_id", "asset-123"},
                             {"symbol", "AAPL"},
                             {"exchange", "NASDAQ"},
                             {"asset_class", "us_equity"},
                             {"qty", "10"},
                             {"qty_available", "5"},
                             {"avg_entry_price", "100"},
                             {"market_value", "150"},
                             {"cost_basis", "1000"},
                             {"unrealized_pl", "50"},
                             {"unrealized_plpc", "0.5"},
                             {"unrealized_intraday_pl", "10"},
                             {"unrealized_intraday_plpc", "0.1"},
                             {"current_price", "150"},
                             {"lastday_price", "140"},
                             {"change_today", "0.07"}};

  const auto position = json.get<alpaca::Position>();

  EXPECT_EQ(position.asset_id, "asset-123");
  EXPECT_EQ(position.symbol, "AAPL");
  EXPECT_EQ(position.exchange, "NASDAQ");
  EXPECT_EQ(position.asset_class, "us_equity");
  EXPECT_EQ(position.qty, "10");
  EXPECT_EQ(position.qty_available, "5");
  EXPECT_EQ(position.avg_entry_price, "100");
  EXPECT_EQ(position.market_value, "150");
  EXPECT_EQ(position.cost_basis, "1000");
  EXPECT_EQ(position.unrealized_pl, "50");
  EXPECT_EQ(position.unrealized_plpc, "0.5");
  EXPECT_EQ(position.unrealized_intraday_pl, "10");
  EXPECT_EQ(position.unrealized_intraday_plpc, "0.1");
  EXPECT_EQ(position.current_price, "150");
  EXPECT_EQ(position.lastday_price, "140");
  EXPECT_EQ(position.change_today, "0.07");
}

TEST(PositionModelTest, ClosePositionRequestBuildsQueryParams) {
  alpaca::ClosePositionRequest request;
  request.quantity = "all";
  request.percentage = 50.5;
  request.time_in_force = alpaca::TimeInForce::IOC;
  request.limit_price = 123.45;
  request.stop_price = 120.0;

  const auto params = request.to_query_params();

  ASSERT_EQ(params.size(), 5U);
  EXPECT_EQ(params[0].first, "qty");
  EXPECT_EQ(params[0].second, "all");
  EXPECT_EQ(params[1].first, "percentage");
  EXPECT_DOUBLE_EQ(std::stod(params[1].second), 50.5);
  EXPECT_EQ(params[2].first, "time_in_force");
  EXPECT_EQ(params[2].second, "ioc");
  EXPECT_EQ(params[3].first, "limit_price");
  EXPECT_DOUBLE_EQ(std::stod(params[3].second), 123.45);
  EXPECT_EQ(params[4].first, "stop_price");
  EXPECT_DOUBLE_EQ(std::stod(params[4].second), 120.0);
}

TEST(PositionModelTest, ClosePositionRequestSkipsUnsetFields) {
  const alpaca::ClosePositionRequest request;

  const auto params = request.to_query_params();

  EXPECT_TRUE(params.empty());
}

TEST(ClockModelTest, FromJsonParsesFields) {
  const alpaca::Json json = {{"is_open", true},
                             {"next_open", "2023-08-01T09:30:00Z"},
                             {"next_close", "2023-08-01T16:00:00Z"},
                             {"timestamp", "2023-08-01T12:00:00Z"}};

  const auto clock = json.get<alpaca::Clock>();

  EXPECT_TRUE(clock.is_open);
  EXPECT_EQ(clock.next_open, "2023-08-01T09:30:00Z");
  EXPECT_EQ(clock.next_close, "2023-08-01T16:00:00Z");
  EXPECT_EQ(clock.timestamp, "2023-08-01T12:00:00Z");
}

}  // namespace

