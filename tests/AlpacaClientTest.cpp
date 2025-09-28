#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include <chrono>

#include "alpaca/AlpacaClient.hpp"
#include "alpaca/Exceptions.hpp"
#include "alpaca/Json.hpp"

namespace {

class ScopedEnvironmentOverride {
  public:
    ScopedEnvironmentOverride(std::string name, std::string value) : name_(std::move(name)) {
        if (auto const *existing = std::getenv(name_.c_str()); existing != nullptr) {
            previous_ = existing;
        }
        set(value.c_str());
    }

    ScopedEnvironmentOverride(ScopedEnvironmentOverride const&) = delete;
    ScopedEnvironmentOverride& operator=(ScopedEnvironmentOverride const&) = delete;

    ~ScopedEnvironmentOverride() {
        if (previous_) {
            set(previous_->c_str());
        } else {
            unset();
        }
    }

  private:
    void set(char const *value) {
#if defined(_WIN32)
        if (value != nullptr) {
            _putenv_s(name_.c_str(), value);
        } else {
            std::string assignment = name_ + "=";
            _putenv(assignment.c_str());
        }
#else
        if (value != nullptr) {
            ::setenv(name_.c_str(), value, 1);
        } else {
            ::unsetenv(name_.c_str());
        }
#endif
    }

    void unset() {
        set(nullptr);
    }

    std::string name_;
    std::optional<std::string> previous_{};
};

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

std::string make_data_url_with_version(alpaca::Configuration const& config, std::string const& version) {
    std::string base = config.data_base_url;
    std::string const suffix{"/v2"};
    if (base.size() >= suffix.size() && base.compare(base.size() - suffix.size(), suffix.size(), suffix) == 0) {
        base.erase(base.size() - suffix.size());
        if (!base.empty() && base.back() == '/') {
            return base + version;
        }
        return base + '/' + version;
    }
    if (!base.empty() && base.back() == '/') {
        return base + version;
    }
    return base + '/' + version;
}

TEST(ConfigurationTest, FromEnvironmentIncludesStreamingEndpoints) {
    auto environment = alpaca::Environments::Paper();
    alpaca::Configuration cfg = alpaca::Configuration::FromEnvironment(environment, "id", "secret");

    EXPECT_EQ(cfg.api_key_id, "id");
    EXPECT_EQ(cfg.api_secret_key, "secret");
    EXPECT_EQ(cfg.trading_base_url, environment.trading_base_url);
    EXPECT_EQ(cfg.data_base_url, environment.data_base_url);
    EXPECT_EQ(cfg.trading_stream_url, environment.trading_stream_url);
    EXPECT_EQ(cfg.market_data_stream_url, environment.market_data_stream_url);
    EXPECT_EQ(cfg.crypto_stream_url, environment.crypto_stream_url);
}

TEST(ConfigurationTest, FromEnvironmentPrefersEnvironmentVariables) {
    ScopedEnvironmentOverride key{"APCA_API_KEY_ID", "env-key"};
    ScopedEnvironmentOverride secret{"APCA_API_SECRET_KEY", "env-secret"};
    ScopedEnvironmentOverride trading{"APCA_API_BASE_URL", "https://env-trading.example"};
    ScopedEnvironmentOverride data{"APCA_API_DATA_URL", "https://env-data.example"};
    ScopedEnvironmentOverride broker{"APCA_API_BROKER_URL", "https://env-broker.example"};
    ScopedEnvironmentOverride trading_stream{"APCA_API_STREAM_URL", "wss://env-trading-stream.example"};
    ScopedEnvironmentOverride market_stream{"APCA_API_DATA_STREAM_URL", "wss://env-market-data-stream.example"};
    ScopedEnvironmentOverride crypto_stream{"APCA_API_CRYPTO_STREAM_URL", "wss://env-crypto-stream.example"};
    ScopedEnvironmentOverride options_stream{"APCA_API_OPTIONS_STREAM_URL", "wss://env-options-stream.example"};

    auto environment = alpaca::Environments::Paper();
    alpaca::Configuration cfg = alpaca::Configuration::FromEnvironment(environment, "arg-key", "arg-secret");

    EXPECT_EQ(cfg.api_key_id, "env-key");
    EXPECT_EQ(cfg.api_secret_key, "env-secret");
    EXPECT_EQ(cfg.trading_base_url, "https://env-trading.example");
    EXPECT_EQ(cfg.data_base_url, "https://env-data.example");
    EXPECT_EQ(cfg.broker_base_url, "https://env-broker.example");
    EXPECT_EQ(cfg.trading_stream_url, "wss://env-trading-stream.example");
    EXPECT_EQ(cfg.market_data_stream_url, "wss://env-market-data-stream.example");
    EXPECT_EQ(cfg.crypto_stream_url, "wss://env-crypto-stream.example");
    EXPECT_EQ(cfg.options_stream_url, "wss://env-options-stream.example");
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

TEST(AlpacaClientTest, SubmitOptionOrderTargetsOptionsEndpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(
    alpaca::HttpResponse{200,
                         R"({"id":"opt-order-1","symbol":"AAPL240119C00195000","side":"buy","type":"market",
             "created_at":"2023-01-01T00:00:00Z","time_in_force":"day","status":"accepted"})",
                         {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::NewOptionOrderRequest request;
    request.symbol = "AAPL240119C00195000";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::MARKET;
    request.time_in_force = alpaca::TimeInForce::DAY;
    request.quantity = "1";

    alpaca::OptionOrder const order = client.submit_option_order(request);
    EXPECT_EQ(order.id, "opt-order-1");
    EXPECT_EQ(order.symbol, "AAPL240119C00195000");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(http_request.url, config.trading_base_url + "/v2/options/orders");

    alpaca::Json const json = alpaca::Json::parse(http_request.body);
    EXPECT_EQ(json.at("symbol"), "AAPL240119C00195000");
    EXPECT_EQ(json.at("qty"), "1");
}

TEST(AlpacaClientTest, ListOptionOrdersExpandsNestedLegsWhenRequested) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"([
            {
                "id": "spread-1",
                "asset_id": "asset-combo",
                "client_order_id": "combo-1",
                "account_id": "account-1",
                "created_at": "2023-01-01T00:00:00Z",
                "symbol": "AAPL24JAN-SPREAD",
                "asset_class": "option",
                "side": "buy",
                "type": "limit",
                "time_in_force": "day",
                "status": "accepted",
                "qty": "1",
                "limit_price": "2.45",
                "legs": [
                    {
                        "id": "leg-1",
                        "asset_id": "asset-leg-1",
                        "client_order_id": "combo-1-leg-1",
                        "account_id": "account-1",
                        "created_at": "2023-01-01T00:00:00Z",
                        "symbol": "AAPL240119C00195000",
                        "asset_class": "option",
                        "side": "buy",
                        "type": "limit",
                        "time_in_force": "day",
                        "status": "filled",
                        "qty": "1",
                        "filled_qty": "1",
                        "limit_price": "5.00",
                        "legs": []
                    },
                    {
                        "id": "leg-2",
                        "asset_id": "asset-leg-2",
                        "client_order_id": "combo-1-leg-2",
                        "account_id": "account-1",
                        "created_at": "2023-01-01T00:00:00Z",
                        "symbol": "AAPL240119C00185000",
                        "asset_class": "option",
                        "side": "sell",
                        "type": "limit",
                        "time_in_force": "day",
                        "status": "filled",
                        "qty": "1",
                        "filled_qty": "1",
                        "limit_price": "2.55",
                        "legs": []
                    }
                ]
            }
        ])",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::ListOptionOrdersRequest request;
    request.nested = true;

    std::vector<alpaca::OptionOrder> const orders = client.list_option_orders(request);
    ASSERT_EQ(orders.size(), 1U);
    alpaca::OptionOrder const& spread = orders.front();
    EXPECT_EQ(spread.id, "spread-1");
    EXPECT_EQ(spread.symbol, "AAPL24JAN-SPREAD");
    ASSERT_EQ(spread.legs.size(), 2U);
    EXPECT_EQ(spread.legs.front().symbol, "AAPL240119C00195000");
    EXPECT_EQ(spread.legs.back().symbol, "AAPL240119C00185000");
    EXPECT_EQ(spread.legs.front().side, alpaca::OrderSide::BUY);
    EXPECT_EQ(spread.legs.back().side, alpaca::OrderSide::SELL);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_TRUE(http_request.url.find(config.trading_base_url + "/v2/options/orders") == 0);
    EXPECT_NE(http_request.url.find("nested=true"), std::string::npos);
}

TEST(AlpacaClientTest, SubmitCryptoOrderSerializesVenueExtensions) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"({"id":"crypto-order-1","symbol":"BTCUSD","side":"buy","type":"limit",
             "created_at":"2023-01-01T00:00:00Z","time_in_force":"ioc","status":"accepted"})",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::NewCryptoOrderRequest request;
    request.symbol = "BTCUSD";
    request.side = alpaca::OrderSide::BUY;
    request.type = alpaca::OrderType::LIMIT;
    request.time_in_force = alpaca::TimeInForce::IOC;
    request.quantity = "0.5";
    request.limit_price = "26000";
    request.quote_symbol = "USD";
    request.venue = "CBSE";
    request.routing_strategy = "smart";
    request.post_only = true;

    alpaca::CryptoOrder const order = client.submit_crypto_order(request);
    EXPECT_EQ(order.id, "crypto-order-1");
    EXPECT_EQ(order.symbol, "BTCUSD");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(http_request.url, config.trading_base_url + "/v2/crypto/orders");

    alpaca::Json const json = alpaca::Json::parse(http_request.body);
    EXPECT_EQ(json.at("symbol"), "BTCUSD");
    EXPECT_EQ(json.at("quote_symbol"), "USD");
    EXPECT_EQ(json.at("venue"), "CBSE");
    EXPECT_EQ(json.at("routing_strategy"), "smart");
    EXPECT_TRUE(json.at("post_only").get<bool>());
}

TEST(AlpacaClientTest, ListCryptoOrdersTargetsCryptoEndpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200, R"([])", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::ListCryptoOrdersRequest request;
    request.limit = 25;

    auto const orders = client.list_crypto_orders(request);
    EXPECT_TRUE(orders.empty());

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_TRUE(http_request.url.find("/v2/crypto/orders") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("asset_class=crypto") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("limit=25") != std::string::npos);
}

TEST(AlpacaClientTest, SubmitOtcOrderSerializesCounterpartyFields) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"({"id":"otc-1","symbol":"AAPL","side":"sell","type":"limit",
             "created_at":"2023-01-01T00:00:00Z","time_in_force":"gtc","status":"accepted"})",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::NewOtcOrderRequest request;
    request.symbol = "AAPL";
    request.side = alpaca::OrderSide::SELL;
    request.type = alpaca::OrderType::LIMIT;
    request.time_in_force = alpaca::TimeInForce::GTC;
    request.quantity = "1000";
    request.limit_price = "175";
    request.counterparty = "desk-42";
    request.quote_id = "qt-1";
    request.settlement_date = "2023-09-15";

    alpaca::OtcOrder const order = client.submit_otc_order(request);
    EXPECT_EQ(order.id, "otc-1");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(http_request.url, config.trading_base_url + "/v2/otc/orders");

    alpaca::Json const json = alpaca::Json::parse(http_request.body);
    EXPECT_EQ(json.at("counterparty"), "desk-42");
    EXPECT_EQ(json.at("quote_id"), "qt-1");
    EXPECT_EQ(json.at("settlement_date"), "2023-09-15");
}

TEST(AlpacaClientTest, ListOptionContractsParsesResponse) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"({
            "contracts": [
                {
                    "id": "contract-1",
                    "symbol": "AAPL240119C00195000",
                    "status": "active",
                    "tradable": true,
                    "underlying_symbol": "AAPL",
                    "expiration_date": "2024-01-19",
                    "strike_price": "195",
                    "type": "call",
                    "style": "american",
                    "root_symbol": "AAPL",
                    "exchange": "OPRA",
                    "exercise_style": "american",
                    "multiplier": "100",
                    "open_interest": 1500,
                    "open_interest_date": "2023-09-14",
                    "close_price": 5.25,
                    "contract_size": "100",
                    "underlying_asset_id": "asset-1"
                }
            ],
            "next_page_token": "cursor"
        })",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::ListOptionContractsRequest request;
    request.underlying_symbols = {"AAPL"};
    request.limit = 1;

    alpaca::OptionContractsResponse const response = client.list_option_contracts(request);
    ASSERT_EQ(response.contracts.size(), 1U);
    EXPECT_EQ(response.next_page_token, std::make_optional<std::string>("cursor"));
    alpaca::OptionContract const& contract = response.contracts.front();
    EXPECT_EQ(contract.symbol, "AAPL240119C00195000");
    EXPECT_EQ(contract.status, alpaca::OptionStatus::ACTIVE);
    EXPECT_TRUE(contract.tradable);
    EXPECT_EQ(contract.type, alpaca::OptionType::CALL);
    EXPECT_EQ(contract.style, alpaca::OptionStyle::AMERICAN);
    EXPECT_EQ(contract.exchange, std::make_optional(alpaca::OptionExchange::OPRA));
    EXPECT_EQ(contract.exercise_style, std::make_optional(alpaca::OptionStyle::AMERICAN));
    EXPECT_EQ(contract.multiplier, std::make_optional<std::string>("100"));
    EXPECT_EQ(contract.open_interest, std::make_optional<std::uint64_t>(1500));
    EXPECT_EQ(contract.open_interest_date, std::make_optional<std::string>("2023-09-14"));
    ASSERT_TRUE(contract.close_price.has_value());
    EXPECT_NEAR(*contract.close_price, 5.25, 1e-9);
    EXPECT_EQ(contract.contract_size, std::make_optional<std::string>("100"));
    EXPECT_EQ(contract.underlying_asset_id, std::make_optional<std::string>("asset-1"));

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_TRUE(http_request.url.find("underlying_symbols=AAPL") != std::string::npos);
    EXPECT_TRUE(http_request.url.find("limit=1") != std::string::npos);
}

TEST(AlpacaClientTest, GetOptionAnalyticsParsesGreeksAndLegs) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"({
            "symbol": "AAPL240119C00195000",
            "greeks": {
                "delta": 0.55,
                "gamma": 0.01,
                "theta": -0.02,
                "vega": 0.05,
                "rho": 0.1
            },
            "risk_parameters": {
                "implied_volatility": 0.25,
                "theoretical_price": 5.23,
                "underlying_price": 190.5,
                "breakeven_price": 200.0
            },
            "implied_volatility": 0.25,
            "legs": [
                {"symbol": "AAPL240119C00195000", "side": "buy", "ratio": 1}
            ]
        })",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::OptionAnalytics const analytics = client.get_option_analytics("AAPL240119C00195000");
    EXPECT_EQ(analytics.symbol, "AAPL240119C00195000");
    ASSERT_TRUE(analytics.greeks.has_value());
    EXPECT_NEAR(*analytics.greeks->delta, 0.55, 1e-9);
    ASSERT_TRUE(analytics.risk_parameters.has_value());
    EXPECT_NEAR(*analytics.risk_parameters->theoretical_price, 5.23, 1e-9);
    ASSERT_EQ(analytics.legs.size(), 1U);
    EXPECT_EQ(analytics.legs.front().side, alpaca::OrderSide::BUY);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.url, config.trading_base_url + "/v2/options/analytics/AAPL240119C00195000");
}

TEST(AlpacaClientTest, ListOptionPositionsParsesExtendedFields) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"([
            {
                "asset_id":"asset-1",
                "account_id":"account-1",
                "symbol":"AAPL240119C00195000",
                "exchange":"OPRA",
                "asset_class":"option",
                "qty":"1",
                "qty_available":"1",
                "avg_entry_price":"1.25",
                "market_value":"125",
                "cost_basis":"125",
                "unrealized_pl":"0",
                "unrealized_plpc":"0",
                "unrealized_intraday_pl":"0",
                "unrealized_intraday_plpc":"0",
                "current_price":"1.25",
                "lastday_price":"1.00",
                "change_today":"0.20",
                "side":"long",
                "contract_multiplier":100,
                "expiry":"2024-01-19",
                "strike_price":"195",
                "style":"american",
                "type":"call",
                "underlying_symbol":"AAPL"
            }
        ])",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    auto const positions = client.list_option_positions();
    ASSERT_EQ(positions.size(), 1U);
    alpaca::OptionPosition const& position = positions.front();
    EXPECT_EQ(position.symbol, "AAPL240119C00195000");
    EXPECT_EQ(position.account_id, "account-1");
    EXPECT_EQ(position.exchange, std::make_optional(alpaca::OptionExchange::OPRA));
    EXPECT_EQ(position.contract_multiplier, std::make_optional<std::string>("100"));
    EXPECT_EQ(position.expiry, std::make_optional<std::string>("2024-01-19"));
    EXPECT_EQ(position.strike_price, std::make_optional<std::string>("195"));
    EXPECT_EQ(position.style, std::make_optional(alpaca::OptionStyle::AMERICAN));
    EXPECT_EQ(position.type, std::make_optional(alpaca::OptionType::CALL));
    EXPECT_EQ(position.underlying_symbol, std::make_optional<std::string>("AAPL"));

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_EQ(http_request.url, config.trading_base_url + "/v2/options/positions");
}

TEST(AlpacaClientTest, ListOptionPositionsRejectsUnknownType) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"([
            {
                "asset_id":"asset-1",
                "account_id":"account-1",
                "symbol":"AAPL240119C00195000",
                "exchange":"OPRA",
                "asset_class":"option",
                "qty":"1",
                "qty_available":"1",
                "avg_entry_price":"1.25",
                "market_value":"125",
                "cost_basis":"125",
                "unrealized_pl":"0",
                "unrealized_plpc":"0",
                "unrealized_intraday_pl":"0",
                "unrealized_intraday_plpc":"0",
                "current_price":"1.25",
                "lastday_price":"1.00",
                "change_today":"0.20",
                "side":"long",
                "contract_multiplier":100,
                "expiry":"2024-01-19",
                "strike_price":"195",
                "style":"american",
                "type":"synthetic",
                "underlying_symbol":"AAPL"
            }
        ])",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    EXPECT_THROW(client.list_option_positions(), alpaca::InvalidArgumentException);
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

TEST(AlpacaClientTest, GetStockAuctionsDelegatesToMarketDataClient) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{200,
                                                R"({
            "auctions": [
                {
                    "symbol": "AAPL",
                    "timestamp": "2024-05-01T13:30:00Z",
                    "auction_type": "closing"
                }
            ]
        })",
                                                {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::HistoricalAuctionsRequest request;
    auto const response = client.get_stock_auctions("AAPL", request);
    ASSERT_EQ(response.auctions.size(), 1U);
    EXPECT_EQ(response.auctions.front().symbol, "AAPL");

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_TRUE(http_request.url.find("/v2/stocks/AAPL/auctions") != std::string::npos);
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
    EXPECT_NE(http_request.url.find("/stocks/AAPL/trades/latest"), std::string::npos);
    EXPECT_NE(http_request.url.find("feed=iex"), std::string::npos);
}

TEST(AlpacaClientTest, LatestCryptoTradeUsesBetaV3Endpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200, R"({"trades":{"BTC/USD":{"i":"t1","x":"CBSE","p":25000.5,"s":1,"t":"2023-01-01T00:00:00Z"}}})", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::LatestCryptoDataRequest request;
    request.symbols = {"BTC/USD"};

    auto const latest = client.get_latest_crypto_trade("us", request);
    ASSERT_EQ(latest.trades.size(), 1U);
    EXPECT_DOUBLE_EQ(latest.trades.at("BTC/USD").price, 25000.5);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    auto const expected_prefix = make_data_url_with_version(config, "v1beta3");
    EXPECT_EQ(http_request.url.substr(0, expected_prefix.size()), expected_prefix);
    EXPECT_NE(http_request.url.find("/v1beta3/crypto/us/latest/trades"), std::string::npos);
    EXPECT_NE(http_request.url.find("symbols=BTC%2FUSD"), std::string::npos);
}

TEST(AlpacaClientTest, LatestCryptoOrderbookSerializesExchanges) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"orderbooks":{"BTC/USD":{"t":"2023-01-01T00:00:00Z","b":[{"p":24999.5,"s":0.75}],"a":[{"p":25000.5,"s":0.5}]}}})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::LatestCryptoOrderbookRequest request;
    request.symbols = {"BTC/USD"};
    request.exchanges = {"CBSE"};

    auto const books = client.get_latest_crypto_orderbook("us", request);
    ASSERT_EQ(books.orderbooks.size(), 1U);
    ASSERT_EQ(books.orderbooks.at("BTC/USD").asks.size(), 1U);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_NE(http_request.url.find("/v1beta3/crypto/us/latest/orderbooks"), std::string::npos);
    EXPECT_NE(http_request.url.find("symbols=BTC%2FUSD"), std::string::npos);
    EXPECT_NE(http_request.url.find("exchanges=CBSE"), std::string::npos);
}

TEST(AlpacaClientTest, LatestCryptoQuoteIncludesCurrencyFilter) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"quotes":{"BTC/USD":{"ax":"CBSE","ap":25001.0,"as":0.5,"bx":"CBSE","bp":24999.0,"bs":0.75,"t":"2023-01-01T00:00:00Z"}}})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::LatestCryptoDataRequest request;
    request.symbols = {"BTC/USD"};
    request.currency = std::string{"USD"};

    auto const quotes = client.get_latest_crypto_quote("us", request);
    ASSERT_EQ(quotes.quotes.size(), 1U);
    EXPECT_DOUBLE_EQ(quotes.quotes.at("BTC/USD").ask_price, 25001.0);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_NE(http_request.url.find("/v1beta3/crypto/us/latest/quotes"), std::string::npos);
    EXPECT_NE(http_request.url.find("symbols=BTC%2FUSD"), std::string::npos);
    EXPECT_NE(http_request.url.find("currency=USD"), std::string::npos);
}

TEST(AlpacaClientTest, LatestCryptoBarUsesBetaV3Endpoint) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200,
        R"({"bars":{"BTC/USD":{"t":"2023-01-01T00:00:00Z","o":24000.0,"h":25500.0,"l":23500.0,"c":25000.0,"v":10}}})",
        {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::LatestCryptoDataRequest request;
    request.symbols = {"BTC/USD"};

    auto const bars = client.get_latest_crypto_bar("us", request);
    ASSERT_EQ(bars.bars.size(), 1U);
    EXPECT_DOUBLE_EQ(bars.bars.at("BTC/USD").close, 25000.0);

    ASSERT_EQ(stub->requests().size(), 1U);
    auto const& http_request = stub->requests().front();
    EXPECT_EQ(http_request.method, alpaca::HttpMethod::GET);
    EXPECT_NE(http_request.url.find("/v1beta3/crypto/us/latest/bars"), std::string::npos);
    EXPECT_NE(http_request.url.find("symbols=BTC%2FUSD"), std::string::npos);
}

TEST(AlpacaClientTest, StockBarsForwardQueryParameters) {
    auto stub = std::make_shared<StubHttpClient>();
    stub->enqueue_response(alpaca::HttpResponse{
        200, R"({"symbol":"AAPL","bars":[{"t":"2023-01-01T00:00:00Z","o":1.0,"h":1.5,"l":0.5,"c":1.2,"v":100}]})", {}});

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::AlpacaClient client(config, stub);

    alpaca::StockBarsRequest request;
    request.timeframe = alpaca::TimeFrame::minute();
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
    request.timeframe = alpaca::TimeFrame::minute();

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
