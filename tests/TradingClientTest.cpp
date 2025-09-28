#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "FakeHttpClient.hpp"
#include "alpaca/Configuration.hpp"
#include "alpaca/Exceptions.hpp"
#include "alpaca/Json.hpp"
#include "alpaca/TradingClient.hpp"

namespace {

alpaca::Json make_order_json(std::string id, std::string status) {
    return alpaca::Json{
        {"id",              id                    },
        {"asset_id",        "asset-" + id         },
        {"client_order_id", "client-" + id        },
        {"account_id",      "account"             },
        {"created_at",      "2023-01-01T00:00:00Z"},
        {"updated_at",      nullptr               },
        {"submitted_at",    nullptr               },
        {"filled_at",       nullptr               },
        {"expired_at",      nullptr               },
        {"canceled_at",     nullptr               },
        {"failed_at",       nullptr               },
        {"replaced_at",     nullptr               },
        {"replaced_by",     ""                    },
        {"replaces",        ""                    },
        {"symbol",          "AAPL"                },
        {"asset_class",     "us_equity"           },
        {"side",            "buy"                 },
        {"type",            "market"              },
        {"time_in_force",   "day"                 },
        {"status",          status                },
        {"extended_hours",  false                 }
    };
}

alpaca::Json make_position_json(std::string symbol) {
    return alpaca::Json{
        {"asset_id",                 "asset-" + symbol},
        {"symbol",                   symbol           },
        {"exchange",                 "NASDAQ"         },
        {"asset_class",              "us_equity"      },
        {"qty",                      "1"              },
        {"qty_available",            "1"              },
        {"avg_entry_price",          "100"            },
        {"market_value",             "100"            },
        {"cost_basis",               "100"            },
        {"unrealized_pl",            "0"              },
        {"unrealized_plpc",          "0"              },
        {"unrealized_intraday_pl",   "0"              },
        {"unrealized_intraday_plpc", "0"              },
        {"current_price",            "100"            },
        {"lastday_price",            "100"            },
        {"change_today",             "0"              }
    };
}

alpaca::Json make_close_success_json(std::string order_id, std::string symbol) {
    return alpaca::Json{
        {"order_id", order_id},
        {"status", 200},
        {"symbol", symbol},
        {"body", make_order_json(order_id, "pending_cancel")}
    };
}

TEST(TradingClientTest, CloseAllPositionsTargetsPositionsEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{200, R"([])", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::CloseAllPositionsRequest request;
    request.cancel_orders = true;

    auto const responses = client.close_all_positions(request);
    EXPECT_TRUE(responses.successful.empty());
    EXPECT_TRUE(responses.failed.empty());

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/positions?cancel_orders=true");
}

TEST(TradingClientTest, CancelAllOrdersRetriesUntilOutstandingResolved) {
    auto http = std::make_shared<FakeHttpClient>();

    alpaca::Json initial = alpaca::Json::array();
    initial.push_back({
        {"id",     "order-1"       },
        {"status", "pending_cancel"}
    });
    initial.push_back({
        {"id",     "order-2" },
        {"status", "canceled"}
    });
    initial.push_back({
        {"id",     "order-3" },
        {"status", "rejected"}
    });
    http->push_response(alpaca::HttpResponse{200, initial.dump(), {}});

    alpaca::Json pending_orders = alpaca::Json::array();
    pending_orders.push_back(make_order_json("order-1", "pending_cancel"));
    http->push_response(alpaca::HttpResponse{200, pending_orders.dump(), {}});

    http->push_response(alpaca::HttpResponse{200, alpaca::Json::array().dump(), {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    auto const result = client.cancel_all_orders();

    ASSERT_EQ(result.successful.size(), 2U);
    ASSERT_EQ(result.failed.size(), 1U);
    EXPECT_TRUE(std::any_of(result.successful.begin(), result.successful.end(), [](auto const& item) {
        return item.id == "order-1";
    }));
    EXPECT_TRUE(std::any_of(result.successful.begin(), result.successful.end(), [](auto const& item) {
        return item.id == "order-2";
    }));
    EXPECT_EQ(result.failed.front().id, "order-3");

    ASSERT_EQ(http->requests().size(), 3U);
    EXPECT_EQ(http->requests()[0].request.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(http->requests()[0].request.url, config.trading_base_url + "/v2/orders");
    EXPECT_EQ(http->requests()[1].request.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(http->requests()[1].request.url, config.trading_base_url + "/v2/orders?status=open&limit=500");
    EXPECT_EQ(http->requests()[2].request.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(http->requests()[2].request.url, config.trading_base_url + "/v2/orders?status=open&limit=500");
}

TEST(TradingClientTest, CloseAllPositionsPollsUntilSymbolsDisappear) {
    auto http = std::make_shared<FakeHttpClient>();

    alpaca::Json initial = alpaca::Json::array();
    initial.push_back(make_close_success_json("order-1", "AAPL"));
    initial.push_back(alpaca::Json{
        {"order_id", nullptr                                                          },
        {"status",   400                                                              },
        {"symbol",   "TSLA"                                                           },
        {"body",     alpaca::Json{{"code", 12345}, {"message", "insufficient shares"}}}
    });
    http->push_response(alpaca::HttpResponse{200, initial.dump(), {}});

    alpaca::Json open_positions = alpaca::Json::array();
    open_positions.push_back(make_position_json("AAPL"));
    http->push_response(alpaca::HttpResponse{200, open_positions.dump(), {}});

    http->push_response(alpaca::HttpResponse{200, alpaca::Json::array().dump(), {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    auto const result = client.close_all_positions();

    ASSERT_EQ(result.successful.size(), 1U);
    EXPECT_EQ(result.successful.front().symbol, std::optional<std::string>{"AAPL"});
    ASSERT_EQ(result.failed.size(), 1U);
    EXPECT_EQ(result.failed.front().symbol, std::optional<std::string>{"TSLA"});

    ASSERT_EQ(http->requests().size(), 3U);
    EXPECT_EQ(http->requests()[0].request.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(http->requests()[0].request.url, config.trading_base_url + "/v2/positions");
    EXPECT_EQ(http->requests()[1].request.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(http->requests()[1].request.url, config.trading_base_url + "/v2/positions");
    EXPECT_EQ(http->requests()[2].request.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(http->requests()[2].request.url, config.trading_base_url + "/v2/positions");
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
    options.retry.max_jitter = std::chrono::milliseconds{0};
    options.retry.retry_after_max = std::chrono::milliseconds{0};

    alpaca::TradingClient client(config, http, options);

    EXPECT_THROW(client.get_account(), alpaca::Exception);
    ASSERT_EQ(http->requests().size(), 1U);
}

TEST(TradingClientTest, ListIntervalCalendarTargetsIntervalEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{200, R"([])", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::CalendarRequest request;
    request.start = std::chrono::sys_days{std::chrono::year{2023} / std::chrono::January / 3};
    request.end = std::chrono::sys_days{std::chrono::year{2023} / std::chrono::January / 4};

    auto const calendar = client.list_interval_calendar(request);
    EXPECT_TRUE(calendar.empty());

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/calendar/interval?start=2023-01-03&end=2023-01-04");
}

TEST(TradingClientTest, ListIntervalCalendarParsesSessionAndTradingWindows) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"([{"date":"2023-01-03","session":{"open":"07:00","close":"20:00"},"trading":{"open":"09:30","close":"16:00"}}])",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    alpaca::CalendarRequest request;
    auto const calendar = client.list_interval_calendar(request);
    ASSERT_EQ(calendar.size(), 1U);
    auto const& day = calendar.front();
    EXPECT_EQ(day.date, "2023-01-03");
    EXPECT_EQ(day.session.open, "07:00");
    EXPECT_EQ(day.session.close, "20:00");
    EXPECT_EQ(day.trading.open, "09:30");
    EXPECT_EQ(day.trading.close, "16:00");
}

TEST(TradingClientTest, AddAssetToWatchlistByNameTargetsNamedEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"wl-1","name":"Tech","account_id":"acct","created_at":"2023-01-01T00:00:00Z","updated_at":"2023-01-02T00:00:00Z","assets":[]})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    auto const watchlist = client.add_asset_to_watchlist_by_name("Tech", "AAPL");
    EXPECT_EQ(watchlist.id, "wl-1");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/watchlists:by_name?name=Tech");

    auto const payload = alpaca::Json::parse(recorded.body);
    EXPECT_EQ(payload.at("symbol"), "AAPL");
}

TEST(TradingClientTest, RemoveAssetFromWatchlistByNameTargetsNamedEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{
        200,
        R"({"id":"wl-1","name":"Tech","account_id":"acct","created_at":"2023-01-01T00:00:00Z","updated_at":"2023-01-02T00:00:00Z","assets":[]})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    auto const watchlist = client.remove_asset_from_watchlist_by_name("Tech", "AAPL");
    EXPECT_EQ(watchlist.id, "wl-1");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/watchlists:by_name/AAPL?name=Tech");
    EXPECT_TRUE(recorded.body.empty());
}

TEST(TradingClientTest, DeleteWatchlistByNameTargetsNamedEndpoint) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(alpaca::HttpResponse{204, std::string{}, {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::TradingClient client(config, http);

    client.delete_watchlist_by_name("Tech");

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& recorded = http->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::DELETE_);
    EXPECT_EQ(recorded.url, config.trading_base_url + "/v2/watchlists:by_name?name=Tech");
    EXPECT_TRUE(recorded.body.empty());
}

} // namespace
