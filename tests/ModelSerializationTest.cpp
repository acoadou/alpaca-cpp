#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>

#include "alpaca/models/Account.hpp"
#include "alpaca/models/AccountActivity.hpp"
#include "alpaca/models/AccountConfiguration.hpp"
#include "alpaca/models/Common.hpp"
#include "alpaca/models/CorporateActions.hpp"
#include "alpaca/models/MarketData.hpp"
#include "alpaca/models/News.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/PortfolioHistory.hpp"
#include "alpaca/models/Watchlist.hpp"

namespace {

TEST(ModelSerializationTest, NewOrderRequestSupportsAdvancedOptions) {
    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::LIMIT;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "10";
    request.limit_price = "150";
    request.stop_price = "140";
    request.client_order_id = "client";
    request.extended_hours = true;
    request.order_class = alpaca::OrderClass::BRACKET;
    request.take_profit = alpaca::TakeProfitParams{"160"};
    request.stop_loss = alpaca::StopLossParams{std::make_optional<std::string>("135"), std::nullopt};
    request.position_intent = alpaca::PositionIntent::OPENING;
    request.legs = {
        {"AAPL240119C00150000", 1, alpaca::OrderSide::BUY, alpaca::PositionIntent::OPENING},
        {"AAPL240119P00150000", 1, alpaca::OrderSide::SELL, alpaca::PositionIntent::CLOSING}
    };

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json["symbol"], "AAPL");
    EXPECT_EQ(json["side"], "buy");
    EXPECT_EQ(json["type"], "limit");
    EXPECT_EQ(json["time_in_force"], "day");
    EXPECT_EQ(json["qty"], "10");
    EXPECT_EQ(json["limit_price"], "150");
    EXPECT_EQ(json["stop_price"], "140");
    EXPECT_EQ(json["client_order_id"], "client");
    EXPECT_EQ(json["order_class"], "bracket");
    EXPECT_TRUE(json["extended_hours"].get<bool>());
    EXPECT_EQ(json["position_intent"], "opening");
    ASSERT_TRUE(json.contains("take_profit"));
    EXPECT_EQ(json["take_profit"]["limit_price"], "160");
    ASSERT_TRUE(json.contains("stop_loss"));
    EXPECT_EQ(json["stop_loss"]["stop_price"], "135");
    EXPECT_FALSE(json["stop_loss"].contains("limit_price"));
    ASSERT_TRUE(json.contains("legs"));
    ASSERT_EQ(json["legs"].size(), 2);
    EXPECT_EQ(json["legs"][0]["symbol"], "AAPL240119C00150000");
    EXPECT_EQ(json["legs"][0]["ratio"], 1);
    EXPECT_EQ(json["legs"][0]["side"], "buy");
    EXPECT_EQ(json["legs"][0]["position_intent"], "opening");
    EXPECT_EQ(json["legs"][1]["side"], "sell");
    EXPECT_EQ(json["legs"][1]["position_intent"], "closing");
}

TEST(ModelSerializationTest, NewOrderRequestSerializesTrailingFields) {
    alpaca::NewOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::SELL;
    request.type = alpaca::OrderType::TRAILING_STOP;
    request.time_in_force = alpaca::TimeInForce::GTC;
    request.trail_price = "1.50";
    request.trail_percent = "0.5";
    request.high_water_mark = "175.00";

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json["type"], "trailing_stop");
    EXPECT_EQ(json["trail_price"], "1.50");
    EXPECT_EQ(json["trail_percent"], "0.5");
    EXPECT_EQ(json["high_water_mark"], "175.00");
}

TEST(ModelSerializationTest, ReplaceOrderRequestSerializesOptionals) {
    alpaca::ReplaceOrderRequest request;
    request.quantity = "5";
    request.limit_price = "100";
    request.stop_price = "95";
    request.extended_hours = true;

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json["qty"], "5");
    EXPECT_EQ(json["limit_price"], "100");
    EXPECT_EQ(json["stop_price"], "95");
    EXPECT_TRUE(json["extended_hours"].get<bool>());
}

TEST(ModelSerializationTest, OrderDeserializesTrailingFields) {
    alpaca::Json json = {
        {"id",               "order-id"                      },
        {"asset_id",         "asset-id"                      },
        {"client_order_id",  "client-id"                     },
        {"account_id",       "account-id"                    },
        {"created_at",       "2023-01-01T00:00:00Z"          },
        {"symbol",           "AAPL"                          },
        {"asset_class",      "us_equity"                     },
        {"side",             "sell"                          },
        {"type",             "trailing_stop"                 },
        {"time_in_force",    "gtc"                           },
        {"status",           "accepted"                      },
        {"trail_price",      "1.50"                          },
        {"trail_percent",    "0.5"                           },
        {"high_water_mark",  "175.00"                        }
    };

    auto const order = json.get<alpaca::Order>();
    ASSERT_TRUE(order.trail_price.has_value());
    EXPECT_EQ(*order.trail_price, "1.50");
    ASSERT_TRUE(order.trail_percent.has_value());
    EXPECT_EQ(*order.trail_percent, "0.5");
    ASSERT_TRUE(order.high_water_mark.has_value());
    EXPECT_EQ(*order.high_water_mark, "175.00");
}

TEST(ModelSerializationTest, AccountDeserializesExtendedFields) {
    alpaca::Json json = {
        {"id",                      "account"   },
        {"account_number",          "123"       },
        {"currency",                "USD"       },
        {"status",                  "ACTIVE"    },
        {"trade_blocked",           false       },
        {"trading_blocked",         false       },
        {"transfers_blocked",       true        },
        {"pattern_day_trader",      true        },
        {"shorting_enabled",        true        },
        {"buying_power",            "1000"      },
        {"regt_buying_power",       "2000"      },
        {"daytrading_buying_power", "1500"      },
        {"non_marginable_buying_power", "750"    },
        {"equity",                  "2000"      },
        {"last_equity",             "1500"      },
        {"cash",                    "500"       },
        {"cash_long",               "400"       },
        {"cash_short",              "100"       },
        {"cash_withdrawable",       "450"       },
        {"portfolio_value",         "2500"      },
        {"long_market_value",       "1000"      },
        {"short_market_value",      "0"         },
        {"initial_margin",          "0"         },
        {"maintenance_margin",      "0"         },
        {"last_maintenance_margin", "0"         },
        {"multiplier",              "4"         },
        {"sma",                     "200"       },
        {"options_buying_power",    "300"       },
        {"created_at",              "2020-01-01"},
        {"daytrade_count",          "3"         }
    };

    auto const account = json.get<alpaca::Account>();
    EXPECT_EQ(account.id, "account");
    EXPECT_EQ(account.account_number, "123");
    EXPECT_TRUE(account.pattern_day_trader);
    EXPECT_EQ(account.portfolio_value, "2500");
    EXPECT_EQ(account.non_marginable_buying_power, "750");
    EXPECT_EQ(account.cash_long, "400");
    EXPECT_EQ(account.cash_short, "100");
    EXPECT_EQ(account.cash_withdrawable, "450");
    EXPECT_EQ(account.sma, "200");
    EXPECT_EQ(account.options_buying_power, "300");
    ASSERT_TRUE(account.daytrade_count.has_value());
    EXPECT_EQ(*account.daytrade_count, "3");
}

TEST(ModelSerializationTest, AccountConfigurationUpdateSerializesOnlyProvidedFields) {
    alpaca::AccountConfigurationUpdateRequest request;
    request.dtbp_check = "both";
    request.no_shorting = true;
    request.ptp_no_exception_entry = true;
    request.max_options_trading_level = alpaca::OptionsTradingLevel::LONG;

    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json.size(), 4);
    EXPECT_EQ(json["dtbp_check"], "both");
    EXPECT_TRUE(json["no_shorting"].get<bool>());
    EXPECT_TRUE(json["ptp_no_exception_entry"].get<bool>());
    EXPECT_EQ(json["max_options_trading_level"].get<int>(), 2);
    EXPECT_FALSE(json.contains("trade_confirm_email"));
}

TEST(ModelSerializationTest, AccountConfigurationRoundTripsOptionsSettings) {
    alpaca::Json json = {
        {"dtbp_check", "both"},
        {"no_shorting", false},
        {"trade_confirm_email", "all"},
        {"suspend_trade", false},
        {"ptp_no_exception_entry", true},
        {"max_options_trading_level", 3}
    };

    auto const configuration = json.get<alpaca::AccountConfiguration>();
    EXPECT_TRUE(configuration.ptp_no_exception_entry);
    ASSERT_TRUE(configuration.max_options_trading_level.has_value());
    EXPECT_EQ(*configuration.max_options_trading_level, alpaca::OptionsTradingLevel::SPREADS);

    alpaca::Json serialized;
    to_json(serialized, configuration);

    EXPECT_TRUE(serialized["ptp_no_exception_entry"].get<bool>());
    EXPECT_EQ(serialized["max_options_trading_level"].get<int>(), 3);
}

TEST(ModelSerializationTest, PortfolioHistoryParsesNumericVectors) {
    alpaca::Json json = {
        {"timestamp",       alpaca::Json::array({1, 2})        },
        {"equity",          alpaca::Json::array({100.0, 110.0})},
        {"profit_loss",     alpaca::Json::array({0.0, 10.0})   },
        {"profit_loss_pct", alpaca::Json::array({0.0, 0.1})    },
        {"base_value",      1000.0                             },
        {"timeframe",       "1D"                               }
    };

    auto const history = json.get<alpaca::PortfolioHistory>();
    EXPECT_EQ(history.timestamp.size(), 2U);
    EXPECT_DOUBLE_EQ(history.equity.back(), 110.0);
    EXPECT_EQ(history.timeframe, "1D");
}

TEST(ModelSerializationTest, WatchlistSerializationIncludesSymbols) {
    alpaca::CreateWatchlistRequest request{
        "My Watchlist", {"AAPL", "MSFT"}
    };
    alpaca::Json json;
    to_json(json, request);

    EXPECT_EQ(json["name"], "My Watchlist");
    ASSERT_EQ(json["symbols"].size(), 2);
    EXPECT_EQ(json["symbols"][0], "AAPL");
}

TEST(ModelSerializationTest, OrderEnumConversionsRoundTrip) {
    EXPECT_EQ(alpaca::to_string(alpaca::OrderSide::SELL), "sell");
    EXPECT_EQ(alpaca::to_string(alpaca::OrderType::TRAILING_STOP), "trailing_stop");
    EXPECT_EQ(alpaca::order_side_from_string("BUY"), alpaca::OrderSide::BUY);
    EXPECT_EQ(alpaca::order_class_from_string("oco"), alpaca::OrderClass::ONE_CANCELS_OTHER);
    EXPECT_EQ(alpaca::time_in_force_from_string("gtc"), alpaca::TimeInForce::GTC);
    EXPECT_EQ(alpaca::asset_class_from_string("us_equity"), alpaca::AssetClass::US_EQUITY);
}

TEST(ModelSerializationTest, PortfolioHistoryRequestBuildsQueryParams) {
    alpaca::PortfolioHistoryRequest request;
    request.period = "1M";
    request.timeframe = "1D";
    request.date_start = "2023-01-01";
    request.extended_hours = true;

    auto const params = request.to_query_params();
    ASSERT_EQ(params.size(), 4U);
    EXPECT_EQ(params[0].first, "period");
    EXPECT_EQ(params[0].second, "1M");
    EXPECT_EQ(params[1].first, "timeframe");
    EXPECT_EQ(params[1].second, "1D");
    EXPECT_EQ(params[2].first, "date_start");
    EXPECT_EQ(params[2].second, "2023-01-01");
    EXPECT_EQ(params[3].first, "extended_hours");
    EXPECT_EQ(params[3].second, "true");
}

TEST(ModelSerializationTest, NewsRequestValidationAndQueryParams) {
    alpaca::NewsRequest request;
    request.symbols = {"AAPL", "MSFT"};
    request.limit = 5;
    request.page_token = std::string{"cursor"};
    request.include_content = true;
    request.exclude_contentless = true;

    auto params = request.to_query_params();
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& pair) {
        return pair.first == "symbols" && pair.second.find("AAPL") != std::string::npos;
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& pair) {
        return pair.first == "limit" && pair.second == "5";
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& pair) {
        return pair.first == "page_token" && pair.second == "cursor";
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& pair) {
        return pair.first == "include_content" && pair.second == "true";
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& pair) {
        return pair.first == "exclude_contentless" && pair.second == "true";
    }),
              params.end());

    request.limit = 0;
    EXPECT_THROW(request.to_query_params(), std::invalid_argument);
}

TEST(ModelSerializationTest, LatestCryptoDataRequestValidatesSymbolsAndCurrency) {
    alpaca::LatestCryptoDataRequest request;
    request.symbols = {"BTC/USD", "ETH/USD"};
    request.currency = std::string{"USD"};

    auto const params = request.to_query_params();
    ASSERT_EQ(params.size(), 2U);
    EXPECT_EQ(params[0].first, "symbols");
    EXPECT_NE(params[0].second.find("BTC/USD"), std::string::npos);
    EXPECT_EQ(params[1].first, "currency");
    EXPECT_EQ(params[1].second, "USD");

    request.symbols.clear();
    EXPECT_THROW(request.to_query_params(), std::invalid_argument);
}

TEST(ModelSerializationTest, LatestCryptoOrderbookRequestSerializesExchanges) {
    alpaca::LatestCryptoOrderbookRequest request;
    request.symbols = {"BTC/USD"};
    request.exchanges = {"CBSE", "ERSX"};

    auto const params = request.to_query_params();
    ASSERT_EQ(params.size(), 2U);
    EXPECT_EQ(params[0].first, "symbols");
    EXPECT_NE(params[0].second.find("BTC/USD"), std::string::npos);
    EXPECT_EQ(params[1].first, "exchanges");
    EXPECT_NE(params[1].second.find("CBSE"), std::string::npos);

    request.symbols.clear();
    EXPECT_THROW(request.to_query_params(), std::invalid_argument);
}

TEST(ModelSerializationTest, HistoricalAuctionsRequestValidatesRange) {
    alpaca::HistoricalAuctionsRequest request;
    request.symbols = {"AAPL", "MSFT"};
    request.limit = 50;
    request.sort = alpaca::SortDirection::DESC;
    request.page_token = std::string{"cursor"};

    auto const start = alpaca::parse_timestamp("2024-05-01T09:30:00Z");
    auto const end = alpaca::parse_timestamp("2024-05-01T16:00:00Z");
    request.start = start;
    request.end = end;

    auto params = request.to_query_params();
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& entry) {
        return entry.first == "symbols" && entry.second.find("AAPL") != std::string::npos;
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& entry) {
        return entry.first == "limit" && entry.second == "50";
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& entry) {
        return entry.first == "sort" && entry.second == "desc";
    }),
              params.end());
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& entry) {
        return entry.first == "page_token" && entry.second == "cursor";
    }),
              params.end());

    request.end = request.start;
    EXPECT_NO_THROW(request.to_query_params());

    auto copy = request;
    copy.end.reset();
    ASSERT_FALSE(copy.end.has_value());
    EXPECT_NO_THROW(copy.to_query_params());

    request.end = start;
    request.start = end;
    EXPECT_THROW(request.to_query_params(), std::invalid_argument);
}

TEST(ModelSerializationTest, LatestCryptoTradesParseSymbolMap) {
    alpaca::Json json = {
        {"trades",
         {{"BTC/USD", {{"i", "t1"}, {"x", "CBSE"}, {"p", 25000.5}, {"s", 1}, {"t", "2023-01-01T00:00:00Z"}}},
          {"ETH/USD", {{"i", "t2"}, {"x", "ERSX"}, {"p", 1800.25}, {"s", 2}, {"t", "2023-01-01T00:00:01Z"}}}}}
    };

    auto const response = json.get<alpaca::LatestCryptoTrades>();
    ASSERT_EQ(response.trades.size(), 2U);
    EXPECT_DOUBLE_EQ(response.trades.at("BTC/USD").price, 25000.5);
    EXPECT_EQ(response.trades.at("ETH/USD").exchange, "ERSX");
}

TEST(ModelSerializationTest, LatestCryptoBarsParseSymbolMap) {
    alpaca::Json json = {
        {"bars",
         {{"BTC/USD",
           {{"t", "2023-01-01T00:00:00Z"}, {"o", 24000.0}, {"h", 25500.0}, {"l", 23500.0}, {"c", 25000.0}, {"v", 10}}},
          {"ETH/USD",
           {{"t", "2023-01-01T00:00:00Z"}, {"o", 1700.0}, {"h", 1850.0}, {"l", 1650.0}, {"c", 1800.0}, {"v", 20}}}}}
    };

    auto const response = json.get<alpaca::LatestCryptoBars>();
    ASSERT_EQ(response.bars.size(), 2U);
    EXPECT_DOUBLE_EQ(response.bars.at("BTC/USD").close, 25000.0);
    EXPECT_EQ(response.bars.at("ETH/USD").volume, 20U);
}

TEST(ModelSerializationTest, LatestCryptoQuotesParseSymbolMap) {
    alpaca::Json json = {
        {"quotes",
         {{"BTC/USD",
           {{"ax", "CBSE"},
            {"ap", 25001.0},
            {"as", 0.5},
            {"bx", "CBSE"},
            {"bp", 24999.0},
            {"bs", 0.75},
            {"t", "2023-01-01T00:00:00Z"}}},
          {"ETH/USD",
           {{"ax", "ERSX"},
            {"ap", 1801.0},
            {"as", 1.5},
            {"bx", "ERSX"},
            {"bp", 1799.0},
            {"bs", 1.0},
            {"t", "2023-01-01T00:00:01Z"}}}}}
    };

    auto const response = json.get<alpaca::LatestCryptoQuotes>();
    ASSERT_EQ(response.quotes.size(), 2U);
    EXPECT_DOUBLE_EQ(response.quotes.at("BTC/USD").ask_price, 25001.0);
    EXPECT_DOUBLE_EQ(response.quotes.at("ETH/USD").bid_price, 1799.0);
}

TEST(ModelSerializationTest, LatestCryptoOrderbooksParseBidsAndAsks) {
    alpaca::Json json = {
        {"orderbooks",
         {{"BTC/USD",
           {{"t", "2023-01-01T00:00:00Z"},
            {"b", alpaca::Json::array({{{"p", 24999.0}, {"s", 0.75}}})},
            {"a", alpaca::Json::array({{{"p", 25001.0}, {"s", 0.5}}})}}}}}
    };

    auto const response = json.get<alpaca::LatestCryptoOrderbooks>();
    ASSERT_EQ(response.orderbooks.size(), 1U);
    auto const& book = response.orderbooks.at("BTC/USD");
    ASSERT_EQ(book.bids.size(), 1U);
    ASSERT_EQ(book.asks.size(), 1U);
    EXPECT_DOUBLE_EQ(book.bids.front().price, 24999.0);
    EXPECT_DOUBLE_EQ(book.asks.front().size, 0.5);
}

TEST(ModelSerializationTest, MultiBarsRequestRequiresSymbols) {
    alpaca::MultiStockBarsRequest request;
    EXPECT_THROW(request.to_query_params(), std::invalid_argument);

    request.symbols = {"AAPL"};
    auto params = request.to_query_params();
    EXPECT_NE(std::find_if(params.begin(), params.end(),
                           [](auto const& pair) {
        return pair.first == "symbols" && pair.second == "AAPL";
    }),
              params.end());
}

TEST(ModelSerializationTest, LatestStockTradeParsesCoreFields) {
    alpaca::Json json = {
        {"symbol", "AAPL"},
        {"trade",
         {{"i", "t1"},
          {"x", "P"},
          {"p", 123.45},
          {"s", 25},
          {"t", "2023-01-01T14:30:00Z"},
          {"c", alpaca::Json::array({"@", "T"})},
          {"z", "C"}}    }
    };

    auto const latest = json.get<alpaca::LatestStockTrade>();
    EXPECT_EQ(latest.symbol, "AAPL");
    EXPECT_EQ(latest.trade.id, "t1");
    EXPECT_EQ(latest.trade.exchange, "P");
    EXPECT_DOUBLE_EQ(latest.trade.price, 123.45);
    EXPECT_EQ(latest.trade.size, 25U);
    EXPECT_EQ(latest.trade.timestamp, alpaca::parse_timestamp("2023-01-01T14:30:00Z"));
    ASSERT_EQ(latest.trade.conditions.size(), 2U);
    EXPECT_EQ(latest.trade.conditions.back(), "T");
    ASSERT_TRUE(latest.trade.tape.has_value());
    EXPECT_EQ(*latest.trade.tape, "C");
}

TEST(ModelSerializationTest, StockBarsParseOptionalFields) {
    alpaca::Json json = {
        {"symbol",          "AAPL"                                      },
        {"bars",            alpaca::Json::array({{{"t", "2023-01-01T14:30:00Z"},
                                       {"o", 120.0},
                                       {"h", 121.0},
                                       {"l", 119.5},
                                       {"c", 120.5},
                                       {"v", 1000},
                                       {"n", 10},
                                       {"vw", 120.25}}})},
        {"next_page_token", nullptr                                     }
    };

    auto const bars = json.get<alpaca::StockBars>();
    EXPECT_EQ(bars.symbol, "AAPL");
    ASSERT_EQ(bars.bars.size(), 1U);
    EXPECT_EQ(bars.bars.front().timestamp, alpaca::parse_timestamp("2023-01-01T14:30:00Z"));
    EXPECT_DOUBLE_EQ(bars.bars.front().open, 120.0);
    EXPECT_EQ(bars.bars.front().trade_count, 10U);
    ASSERT_TRUE(bars.bars.front().vwap.has_value());
    EXPECT_DOUBLE_EQ(*bars.bars.front().vwap, 120.25);
    EXPECT_FALSE(bars.next_page_token.has_value());
}

TEST(ModelSerializationTest, StockSnapshotHandlesMissingAggregates) {
    alpaca::Json json = {
        {"symbol",       "MSFT"                                                                                 },
        {"latestTrade",  nullptr                                                                                },
        {"latestQuote",
         {{"ax", "P"},
          {"ap", 250.5},
          {"as", 100},
          {"bx", "Q"},
          {"bp", 250.4},
          {"bs", 200},
          {"t", "2023-01-01T14:30:00Z"},
          {"c", alpaca::Json::array({"R"})}}                                                                    },
        {"minuteBar",    nullptr                                                                                },
        {"dailyBar",     nullptr                                                                                },
        {"prevDailyBar",
         {{"t", "2022-12-30T21:00:00Z"}, {"o", 240.0}, {"h", 242.0}, {"l", 238.0}, {"c", 241.5}, {"v", 1500000}}}
    };

    auto const snapshot = json.get<alpaca::StockSnapshot>();
    EXPECT_EQ(snapshot.symbol, "MSFT");
    EXPECT_FALSE(snapshot.latest_trade.has_value());
    ASSERT_TRUE(snapshot.latest_quote.has_value());
    EXPECT_EQ(snapshot.latest_quote->ask_exchange, "P");
    EXPECT_EQ(snapshot.latest_quote->timestamp, alpaca::parse_timestamp("2023-01-01T14:30:00Z"));
    EXPECT_FALSE(snapshot.minute_bar.has_value());
    ASSERT_TRUE(snapshot.previous_daily_bar.has_value());
    EXPECT_EQ(snapshot.previous_daily_bar->timestamp, alpaca::parse_timestamp("2022-12-30T21:00:00Z"));
    EXPECT_DOUBLE_EQ(snapshot.previous_daily_bar->close, 241.5);
}

TEST(ModelSerializationTest, ListOrdersRequestBuildsQueryParams) {
    alpaca::ListOrdersRequest request;
    request.status = alpaca::OrderStatusFilter::OPEN;
    request.limit = 50;
    request.after = alpaca::parse_timestamp("2023-01-01T00:00:00Z");
    request.direction = alpaca::SortDirection::ASC;
    request.side = alpaca::OrderSide::SELL;
    request.nested = true;
    request.symbols = {"AAPL", "MSFT"};

    auto const params = request.to_query_params();
    ASSERT_EQ(params.size(), 7U);
    EXPECT_EQ(params[0].first, "status");
    EXPECT_EQ(params[0].second, "open");
    EXPECT_EQ(params[1].first, "limit");
    EXPECT_EQ(params[1].second, "50");
    EXPECT_EQ(params[2].first, "after");
    EXPECT_EQ(params[2].second, "2023-01-01T00:00:00Z");
    EXPECT_EQ(params[3].first, "direction");
    EXPECT_EQ(params[3].second, "asc");
    EXPECT_EQ(params[4].first, "side");
    EXPECT_EQ(params[4].second, "sell");
    EXPECT_EQ(params[5].first, "nested");
    EXPECT_EQ(params[5].second, "true");
    EXPECT_EQ(params[6].first, "symbols");
    EXPECT_EQ(params[6].second, "AAPL,MSFT");
}

TEST(ModelSerializationTest, OptionChainRequestIncludesRootSymbolInQueryParams) {
    alpaca::OptionChainRequest request;
    request.root_symbol = "AAPL";

    auto const params = request.to_query_params();
    auto const it = std::find_if(params.begin(), params.end(), [](auto const& pair) {
        return pair.first == "root_symbol";
    });

    ASSERT_NE(it, params.end());
    EXPECT_EQ(it->second, "AAPL");
}

TEST(ModelSerializationTest, AccountActivitiesRequestSerializesChronoFields) {
    alpaca::AccountActivitiesRequest request;
    request.activity_types = {"FILL", "FEE"};
    request.date = std::chrono::sys_days{std::chrono::year{2023} / std::chrono::March / 15};
    request.until = alpaca::parse_timestamp("2023-03-16T00:00:00Z");
    request.direction = alpaca::SortDirection::DESC;
    request.page_size = 25;

    auto const params = request.to_query_params();
    ASSERT_EQ(params.size(), 5U);
    EXPECT_EQ(params[0].first, "activity_types");
    EXPECT_EQ(params[0].second, "FILL,FEE");
    EXPECT_EQ(params[1].first, "date");
    EXPECT_EQ(params[1].second, "2023-03-15");
    EXPECT_EQ(params[2].first, "until");
    EXPECT_EQ(params[2].second, "2023-03-16T00:00:00Z");
    EXPECT_EQ(params[3].first, "direction");
    EXPECT_EQ(params[3].second, "desc");
    EXPECT_EQ(params[4].first, "page_size");
    EXPECT_EQ(params[4].second, "25");
}

TEST(ModelSerializationTest, NewsResponseParsesArticles) {
    alpaca::Json json = {
        {"news",
         alpaca::Json::array(
         {{{"id", 1234},
           {"headline", "Headline"},
           {"author", "Reporter"},
           {"summary", "Summary"},
           {"content", "Full content"},
           {"url", "https://example.com"},
           {"source", "benzinga"},
           {"symbols", alpaca::Json::array({"AAPL", "MSFT"})},
           {"images", alpaca::Json::array({{{"url", "https://img"}, {"caption", "Caption"}, {"size", "large"}}})},
           {"created_at", "2023-01-01T00:00:00Z"},
           {"updated_at", "2023-01-01T01:00:00Z"}}})},
        {"next_page_token", "token"                 }
    };

    auto const response = json.get<alpaca::NewsResponse>();
    ASSERT_EQ(response.news.size(), 1U);
    auto const& article = response.news.front();
    EXPECT_EQ(article.id, "1234");
    EXPECT_EQ(article.headline, "Headline");
    ASSERT_TRUE(article.author.has_value());
    EXPECT_EQ(*article.author, "Reporter");
    ASSERT_EQ(article.symbols.size(), 2U);
    ASSERT_EQ(article.images.size(), 1U);
    EXPECT_EQ(article.images.front().url, "https://img");
    ASSERT_TRUE(article.created_at.has_value());
    EXPECT_EQ(alpaca::format_timestamp(*article.created_at), "2023-01-01T00:00:00Z");
    ASSERT_TRUE(response.next_page_token.has_value());
    EXPECT_EQ(*response.next_page_token, "token");
}

TEST(ModelSerializationTest, CorporateActionAnnouncementsParseCoreFields) {
    alpaca::Json json = {
        {"announcements",   alpaca::Json::array({{{"id", "ann-1"},
                                                {"corporate_action_id", "ca-1"},
                                                {"ca_type", "cash_dividend"},
                                                {"ca_sub_type", "regular_cash"},
                                                {"initiating_symbol", "AAPL"},
                                                {"cash", "0.82"},
                                                {"record_date", "2023-01-01"}}})},
        {"next_page_token", nullptr                                                                      }
    };

    auto const response = json.get<alpaca::CorporateActionAnnouncementsResponse>();
    ASSERT_EQ(response.announcements.size(), 1U);
    auto const& announcement = response.announcements.front();
    EXPECT_EQ(announcement.id, "ann-1");
    EXPECT_EQ(announcement.corporate_action_id, "ca-1");
    EXPECT_EQ(announcement.type, "cash_dividend");
    EXPECT_EQ(announcement.sub_type, "regular_cash");
    EXPECT_EQ(announcement.initiating_symbol, "AAPL");
    ASSERT_TRUE(announcement.cash.has_value());
    EXPECT_EQ(*announcement.cash, "0.82");
    ASSERT_TRUE(announcement.record_date.has_value());
    EXPECT_EQ(*announcement.record_date, "2023-01-01");
    EXPECT_FALSE(response.next_page_token.has_value());
}

TEST(ModelSerializationTest, CorporateActionEventsParseCoreFields) {
    alpaca::Json json = {
        {"events",          alpaca::Json::array({{{"id", "evt-1"},
                                         {"corporate_action_id", "ca-1"},
                                         {"ca_type", "cash_dividend"},
                                         {"ca_sub_type", "regular_cash"},
                                         {"symbol", "AAPL"},
                                         {"status", "pending"},
                                         {"effective_date", "2023-01-02"},
                                         {"created_at", "2023-01-01T00:00:00Z"}}})},
        {"next_page_token", "next"                                                                  }
    };

    auto const response = json.get<alpaca::CorporateActionEventsResponse>();
    ASSERT_EQ(response.events.size(), 1U);
    auto const& event = response.events.front();
    EXPECT_EQ(event.id, "evt-1");
    EXPECT_EQ(event.corporate_action_id, "ca-1");
    EXPECT_EQ(event.type, "cash_dividend");
    EXPECT_EQ(event.symbol, "AAPL");
    ASSERT_TRUE(event.status.has_value());
    EXPECT_EQ(*event.status, "pending");
    ASSERT_TRUE(event.created_at.has_value());
    EXPECT_EQ(alpaca::format_timestamp(*event.created_at), "2023-01-01T00:00:00Z");
    ASSERT_TRUE(response.next_page_token.has_value());
    EXPECT_EQ(*response.next_page_token, "next");
}

TEST(ModelSerializationTest, MultiStockResponsesMapSymbols) {
    alpaca::Json json = {
        {"bars",
         {{"AAPL", alpaca::Json::array(
                   {{{"t", "2023-01-01T00:00:00Z"}, {"o", 1.0}, {"h", 2.0}, {"l", 0.5}, {"c", 1.5}, {"v", 1000}}})},
          {"MSFT", alpaca::Json::array(
                   {{{"t", "2023-01-01T00:01:00Z"}, {"o", 2.0}, {"h", 2.5}, {"l", 1.5}, {"c", 2.2}, {"v", 500}}})}}   },
        {"quotes",
         {{"AAPL", alpaca::Json::array({{{"ax", "P"},
                                         {"ap", 1.1},
                                         {"as", 100},
                                         {"bx", "P"},
                                         {"bp", 1.0},
                                         {"bs", 90},
                                         {"t", "2023-01-01T00:00:00Z"}}})}}                                           },
        {"trades",
         {{"MSFT",
           alpaca::Json::array({{{"i", "trade"}, {"x", "P"}, {"p", 2.1}, {"s", 25}, {"t", "2023-01-01T00:00:00Z"}}})}}},
        {"next_page_token", "token"                                                                                   }
    };

    auto const bars = json.get<alpaca::MultiStockBars>();
    ASSERT_EQ(bars.bars.size(), 2U);
    ASSERT_TRUE(bars.bars.contains("AAPL"));
    EXPECT_EQ(bars.bars.at("AAPL").size(), 1U);

    auto const quotes = json.get<alpaca::MultiStockQuotes>();
    ASSERT_EQ(quotes.quotes.size(), 1U);
    EXPECT_DOUBLE_EQ(quotes.quotes.at("AAPL").front().ask_price, 1.1);

    auto const trades = json.get<alpaca::MultiStockTrades>();
    ASSERT_EQ(trades.trades.size(), 1U);
    EXPECT_DOUBLE_EQ(trades.trades.at("MSFT").front().price, 2.1);
    ASSERT_TRUE(trades.next_page_token.has_value());
    EXPECT_EQ(*trades.next_page_token, "token");
}

} // namespace
