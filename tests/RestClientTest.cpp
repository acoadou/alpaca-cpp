#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "FakeHttpClient.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Account.hpp"
#include "alpaca/models/AccountConfiguration.hpp"
#include "alpaca/models/Watchlist.hpp"

namespace {

using ::testing::Eq;
using ::testing::SizeIs;

TEST(RestClientTest, AddsAuthenticationHeaders) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id",                "test"  },
        {"currency",          "USD"   },
        {"status",            "ACTIVE"},
        {"trade_blocked",     false   },
        {"trading_blocked",   false   },
        {"transfers_blocked", false   },
        {"buying_power",      "1000"  },
        {"equity",            "1000"  },
        {"last_equity",       "1000"  }
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    alpaca::Account account = client.get<alpaca::Account>("/v2/account");

    EXPECT_EQ(account.id, "test");
    EXPECT_EQ(account.currency, "USD");

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.headers.at("APCA-API-KEY-ID"), Eq("key"));
    EXPECT_THAT(request.headers.at("APCA-API-SECRET-KEY"), Eq("secret"));
    EXPECT_THAT(request.method, Eq(alpaca::HttpMethod::GET));
    EXPECT_THAT(request.url, Eq(config.trading_base_url + "/v2/account"));
    EXPECT_TRUE(request.verify_peer);
    EXPECT_TRUE(request.verify_host);
}

TEST(RestClientTest, PropagatesApiErrors) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "failure"}
    };
    fake_client->push_response(alpaca::HttpResponse{422, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::ApiException);
}

TEST(RestClientTest, SupportsPatchRequests) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json response_json = {
        {"dtbp_check",          "both"},
        {"no_shorting",         false },
        {"trade_confirm_email", "none"},
        {"suspend_trade",       false }
    };
    fake_client->push_response(alpaca::HttpResponse{200, response_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    alpaca::Json payload = {
        {"dtbp_check", "both"}
    };
    alpaca::AccountConfiguration configuration =
    client.patch<alpaca::AccountConfiguration>("/v2/account/configurations", payload);

    EXPECT_EQ(configuration.dtbp_check, "both");

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_EQ(request.method, alpaca::HttpMethod::PATCH);
    EXPECT_THAT(request.body, Eq(payload.dump()));
}

TEST(RestClientTest, PutRequestsIncludePayload) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json response_json = {
        {"id",         "watch"              },
        {"name",       "My Watchlist"       },
        {"account_id", "acc"                },
        {"assets",     alpaca::Json::array()}
    };
    fake_client->push_response(alpaca::HttpResponse{200, response_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    alpaca::Json payload = {
        {"name", "My Watchlist"}
    };
    auto watchlist = client.put<alpaca::Watchlist>("/v2/watchlists/1", payload);

    EXPECT_EQ(watchlist.name, "My Watchlist");
    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_EQ(request.method, alpaca::HttpMethod::PUT);
    EXPECT_THAT(request.body, Eq(payload.dump()));
}

TEST(RestClientTest, BrokerRequestsUseConfiguredBaseUrl) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    fake_client->push_response(alpaca::HttpResponse{200, alpaca::Json::object().dump(), {}});

    alpaca::RestClient client(config, fake_client, config.broker_base_url);
    EXPECT_NO_THROW(client.get<alpaca::Json>("/v1/accounts"));

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.url, Eq(config.broker_base_url + "/v1/accounts"));
}

TEST(RestClientTest, PropagatesTlsOverrides) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    config.verify_ssl = false;
    config.verify_hostname = false;
    config.ca_bundle_path = "/tmp/custom.pem";
    config.ca_bundle_dir = "/tmp/certs";

    auto fake_client = std::make_shared<FakeHttpClient>();
    fake_client->push_response(alpaca::HttpResponse{204, {}, {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_NO_THROW(client.del("/v2/watchlists/1"));

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_FALSE(request.verify_peer);
    EXPECT_FALSE(request.verify_host);
    EXPECT_EQ(request.ca_bundle_path, config.ca_bundle_path);
    EXPECT_EQ(request.ca_bundle_dir, config.ca_bundle_dir);
}

TEST(RestClientTest, AllowsBearerTokenAuthentication) {
    alpaca::Configuration config;
    config.bearer_token = std::string{"oauth-token"};

    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id",                "test"  },
        {"currency",          "USD"   },
        {"status",            "ACTIVE"},
        {"trade_blocked",     false   },
        {"trading_blocked",   false   },
        {"transfers_blocked", false   },
        {"buying_power",      "1000"  },
        {"equity",            "1000"  },
        {"last_equity",       "1000"  }
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.headers.at("Authorization"), Eq("Bearer oauth-token"));
    EXPECT_EQ(request.headers.count("APCA-API-KEY-ID"), 0);
    EXPECT_EQ(request.headers.count("APCA-API-SECRET-KEY"), 0);
}

TEST(RestClientTest, RespectsCustomAuthorizationHeader) {
    alpaca::Configuration config;
    config.default_headers.emplace("Authorization", "Bearer external-token");

    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id",                "test"  },
        {"currency",          "USD"   },
        {"status",            "ACTIVE"},
        {"trade_blocked",     false   },
        {"trading_blocked",   false   },
        {"transfers_blocked", false   },
        {"buying_power",      "1000"  },
        {"equity",            "1000"  },
        {"last_equity",       "1000"  }
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.headers.at("Authorization"), Eq("Bearer external-token"));
    EXPECT_EQ(request.headers.count("APCA-API-KEY-ID"), 0);
    EXPECT_EQ(request.headers.count("APCA-API-SECRET-KEY"), 0);
}

TEST(RestClientTest, RespectsCustomUserAgentHeader) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    config.default_headers.emplace("User-Agent", "custom-agent/1.0");

    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id",                "test"  },
        {"currency",          "USD"   },
        {"status",            "ACTIVE"},
        {"trade_blocked",     false   },
        {"trading_blocked",   false   },
        {"transfers_blocked", false   },
        {"buying_power",      "1000"  },
        {"equity",            "1000"  },
        {"last_equity",       "1000"  }
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.headers.at("User-Agent"), Eq("custom-agent/1.0"));
}

} // namespace
