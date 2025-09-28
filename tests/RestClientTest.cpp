#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <optional>

#include "FakeHttpClient.hpp"
#include "alpaca/Exceptions.hpp"
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

TEST(RestClientTest, DefaultRetryOptionsFollowAlpacaDefaults) {
    alpaca::RestClient::RetryOptions const defaults = alpaca::RestClient::default_retry_options();

    EXPECT_GE(defaults.max_attempts, 3U);
    EXPECT_EQ(defaults.initial_backoff, std::chrono::milliseconds{100});
    EXPECT_DOUBLE_EQ(defaults.backoff_multiplier, 2.0);
    EXPECT_EQ(defaults.max_backoff, std::chrono::seconds{5});
    EXPECT_EQ(defaults.max_jitter, std::chrono::milliseconds{250});
    EXPECT_EQ(defaults.retry_after_max, std::chrono::seconds{30});
    EXPECT_THAT(defaults.retry_status_codes, ::testing::Contains(429));
}

TEST(RestClientTest, RetriesFailedRequests) {
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

    fake_client->push_response(alpaca::HttpResponse{500, alpaca::Json{{"message", "fail"}}.dump(), {}});
    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient::Options options;
    options.retry.max_attempts = 2;
    options.retry.initial_backoff = std::chrono::milliseconds{0};
    options.retry.max_backoff = std::chrono::milliseconds{0};
    options.retry.max_jitter = std::chrono::milliseconds{0};
    options.retry.retry_after_max = std::chrono::milliseconds{0};
    options.retry.retry_status_codes = {500};

    alpaca::RestClient client(config, fake_client, config.trading_base_url, options);
    alpaca::Account account = client.get<alpaca::Account>("/v2/account");

    EXPECT_EQ(account.id, "test");
    ASSERT_THAT(fake_client->requests(), SizeIs(2));
}

TEST(RestClientTest, DefaultRetriesCoverMultipleAttempts) {
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

    fake_client->push_response(alpaca::HttpResponse{500, alpaca::Json{{"message", "fail"}}.dump(), {}});
    fake_client->push_response(alpaca::HttpResponse{500, alpaca::Json{{"message", "fail"}}.dump(), {}});
    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    alpaca::Account account = client.get<alpaca::Account>("/v2/account");

    EXPECT_EQ(account.id, "test");
    ASSERT_THAT(fake_client->requests(), SizeIs(3));
}

TEST(RestClientTest, RetriesNetworkErrorsForIdempotentRequests) {
    class FlakyHttpClient : public alpaca::HttpClient {
      public:
        explicit FlakyHttpClient(alpaca::HttpResponse success) : success_(std::move(success)) {
        }

        alpaca::HttpResponse send(alpaca::HttpRequest const& request) override {
            requests.push_back(request);
            if (!failed_) {
                failed_ = true;
                throw alpaca::CurlException(alpaca::ErrorCode::CurlPerformFailure, "connection reset", "send");
            }
            return success_;
        }

        std::vector<alpaca::HttpRequest> requests;

      private:
        bool failed_{false};
        alpaca::HttpResponse success_;
    };

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::Json account_json = {
        {"id", "network"},
    };

    auto flaky = std::make_shared<FlakyHttpClient>(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient::Options options;
    options.retry.initial_backoff = std::chrono::milliseconds{0};
    options.retry.max_backoff = std::chrono::milliseconds{0};
    options.retry.max_jitter = std::chrono::milliseconds{0};
    options.retry.retry_after_max = std::chrono::milliseconds{0};

    alpaca::RestClient client(config, flaky, config.trading_base_url, options);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));
    EXPECT_THAT(flaky->requests, SizeIs(2));
}

TEST(RestClientTest, DoesNotRetryNonIdempotentRequestsByDefault) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json order_json = {
        {"id", "post"},
    };

    fake_client->push_response(alpaca::HttpResponse{500, alpaca::Json{{"message", "fail"}}.dump(), {}});
    fake_client->push_response(alpaca::HttpResponse{200, order_json.dump(), {}});

    alpaca::RestClient::Options options;
    options.retry.initial_backoff = std::chrono::milliseconds{0};
    options.retry.max_backoff = std::chrono::milliseconds{0};
    options.retry.max_jitter = std::chrono::milliseconds{0};
    options.retry.retry_after_max = std::chrono::milliseconds{0};
    options.retry.retry_status_codes = {500};

    alpaca::RestClient client(config, fake_client, config.trading_base_url, options);
    EXPECT_THROW(client.post<alpaca::Account>("/v2/account", alpaca::Json::object()), alpaca::ServerException);
    ASSERT_THAT(fake_client->requests(), SizeIs(1));
}

TEST(RestClientTest, RespectsRetryAfterCeiling) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id", "retry-after"},
    };

    alpaca::HttpHeaders headers{
        {"Retry-After", "60"}
    };

    fake_client->push_response(alpaca::HttpResponse{429, alpaca::Json{{"message", "slow"}}.dump(), headers});
    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient::Options options;
    options.retry.initial_backoff = std::chrono::milliseconds{0};
    options.retry.max_backoff = std::chrono::milliseconds{0};
    options.retry.max_jitter = std::chrono::milliseconds{0};
    options.retry.retry_after_max = std::chrono::milliseconds{0};

    alpaca::RestClient client(config, fake_client, config.trading_base_url, options);

    auto const start = std::chrono::steady_clock::now();
    alpaca::Account account = client.get<alpaca::Account>("/v2/account");
    auto const elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(account.id, "retry-after");
    ASSERT_THAT(fake_client->requests(), SizeIs(2));
    EXPECT_LT(elapsed, std::chrono::milliseconds{500});

    ASSERT_THAT(fake_client->requests(), SizeIs(2));
    auto const& first = fake_client->requests()[0];
    auto const& second = fake_client->requests()[1];
    EXPECT_LT(second.timestamp - first.timestamp, std::chrono::milliseconds{500});
}

TEST(RestClientTest, InvokesRequestInterceptors) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id", "test"},
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    int pre_invocations = 0;
    bool post_invoked = false;

    alpaca::RestClient::Options options;
    options.pre_request_hook = [&](alpaca::HttpRequest& request) {
        ++pre_invocations;
        request.headers.emplace("X-Trace", "trace-id");
    };
    options.post_request_hook = [&](alpaca::HttpRequest const& request, alpaca::HttpResponse const& response) {
        post_invoked = true;
        EXPECT_THAT(request.headers.at("X-Trace"), Eq("trace-id"));
        EXPECT_EQ(response.status_code, 200);
    };

    alpaca::RestClient client(config, fake_client, config.trading_base_url, options);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));

    EXPECT_EQ(pre_invocations, 1);
    EXPECT_TRUE(post_invoked);

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.headers.at("X-Trace"), Eq("trace-id"));
}

TEST(RestClientTest, CapturesRateLimitHeaders) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id", "limit-test"}
    };

    alpaca::HttpHeaders headers{
        {"X-RateLimit-Limit",     "200"       },
        {"X-RateLimit-Remaining", "198"       },
        {"X-RateLimit-Used",      "2"         },
        {"X-RateLimit-Reset",     "1700000000"}
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), headers});

    bool handler_invoked = false;
    alpaca::RestClient::RateLimitStatus observed{};

    alpaca::RestClient::Options options;
    options.rate_limit_handler = [&](alpaca::RestClient::RateLimitStatus const& status) {
        handler_invoked = true;
        observed = status;
    };

    alpaca::RestClient client(config, fake_client, config.trading_base_url, options);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));

    EXPECT_TRUE(handler_invoked);
    auto const last = client.last_rate_limit_status();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(last->limit, std::make_optional<long>(200));
    EXPECT_EQ(last->remaining, std::make_optional<long>(198));
    EXPECT_EQ(last->used, std::make_optional<long>(2));
    ASSERT_TRUE(last->reset.has_value());
    ASSERT_TRUE(observed.reset.has_value());
    EXPECT_EQ(last->reset->time_since_epoch().count(), observed.reset->time_since_epoch().count());
}

TEST(RestClientTest, ReturnsRawJsonResponses) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id", "raw"}
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    std::optional<std::string> raw = client.get_raw("/v2/account");

    ASSERT_TRUE(raw.has_value());
    EXPECT_THAT(*raw, Eq(account_json.dump()));
}

TEST(RestClientTest, SupportsCustomAuthenticationHandler) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id", "custom"}
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient::Options options;
    options.auth_handler = [](alpaca::HttpRequest& request, alpaca::Configuration const&) {
        request.headers["Authorization"] = "Custom token";
    };

    alpaca::RestClient client(config, fake_client, config.trading_base_url, options);
    EXPECT_NO_THROW(client.get<alpaca::Account>("/v2/account"));

    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.headers.at("Authorization"), Eq("Custom token"));
    EXPECT_EQ(request.headers.count("APCA-API-KEY-ID"), 0);
    EXPECT_EQ(request.headers.count("APCA-API-SECRET-KEY"), 0);
}

TEST(RestClientTest, SupportsAsyncGetRequests) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json account_json = {
        {"id",       "async" },
        {"currency", "USD"   },
        {"status",   "ACTIVE"}
    };

    fake_client->push_response(alpaca::HttpResponse{200, account_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    auto future = client.get_async<alpaca::Account>("/v2/account");
    alpaca::Account account = future.get();

    EXPECT_EQ(account.id, "async");
    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    EXPECT_THAT(fake_client->requests().front().request.method, Eq(alpaca::HttpMethod::GET));
}

TEST(RestClientTest, SupportsAsyncPostRequests) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();

    alpaca::Json watchlist_json = {
        {"id",         "wl-1"                },
        {"name",       "Growth"              },
        {"account_id", "acct"                },
        {"created_at", "2024-05-01T00:00:00Z"},
        {"updated_at", "2024-05-01T00:00:00Z"},
        {"assets",     alpaca::Json::array() }
    };

    fake_client->push_response(alpaca::HttpResponse{200, watchlist_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);

    alpaca::Json payload = {
        {"name",    "Growth"                             },
        {"symbols", alpaca::Json::array({"AAPL", "MSFT"})}
    };

    auto future = client.post_async<alpaca::Watchlist>("/v2/watchlists", payload);
    alpaca::Watchlist watchlist = future.get();

    EXPECT_EQ(watchlist.id, "wl-1");
    ASSERT_THAT(fake_client->requests(), SizeIs(1));
    auto const& request = fake_client->requests().front().request;
    EXPECT_THAT(request.method, Eq(alpaca::HttpMethod::POST));
    EXPECT_THAT(request.body, Eq(payload.dump()));
}

TEST(RestClientTest, ThrowsValidationException) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "Request invalid" },
        {"code",    "validation_error"}
    };
    fake_client->push_response(alpaca::HttpResponse{422, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::ValidationException);
}

TEST(RestClientTest, ThrowsAuthenticationException) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "API key invalid"     },
        {"code",    "authentication_error"}
    };
    fake_client->push_response(alpaca::HttpResponse{401, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::AuthenticationException);
}

TEST(RestClientTest, ThrowsPermissionException) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "Forbidden"        },
        {"code",    "permission_denied"}
    };
    fake_client->push_response(alpaca::HttpResponse{403, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::PermissionException);
}

TEST(RestClientTest, ThrowsNotFoundException) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "Resource missing"},
        {"code",    "not_found"       }
    };
    fake_client->push_response(alpaca::HttpResponse{404, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::NotFoundException);
}

TEST(RestClientTest, ThrowsRateLimitExceptionFromErrorCode) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "Slow down" },
        {"code",    "rate_limit"}
    };
    fake_client->push_response(alpaca::HttpResponse{400, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::RateLimitException);
}

TEST(RestClientTest, ThrowsClientExceptionForUnhandled4xx) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "Conflict"}
    };
    fake_client->push_response(alpaca::HttpResponse{409, error_json.dump(), {}});

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::ClientException);
}

TEST(RestClientTest, ThrowsServerExceptionFor5xx) {
    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    auto fake_client = std::make_shared<FakeHttpClient>();
    alpaca::Json error_json = {
        {"message", "Service unavailable"}
    };
    auto const retry_attempts = alpaca::RestClient::default_retry_options().max_attempts;
    for (std::size_t i = 0; i < retry_attempts; ++i) {
        fake_client->push_response(alpaca::HttpResponse{503, error_json.dump(), {}});
    }

    alpaca::RestClient client(config, fake_client, config.trading_base_url);
    EXPECT_THROW(client.get<alpaca::Account>("/v2/account"), alpaca::ServerException);
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
