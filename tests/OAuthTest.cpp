#include "alpaca/OAuth.hpp"

#include <gtest/gtest.h>

#include "FakeHttpClient.hpp"

using alpaca::AuthorizationCodeTokenRequest;
using alpaca::AuthorizationUrlRequest;
using alpaca::OAuthClient;
using alpaca::OAuthTokenResponse;
using alpaca::PkcePair;
using alpaca::RefreshTokenRequest;

TEST(OAuthTest, GeneratePkcePairProducesValidLengths) {
    PkcePair const pkce = alpaca::GeneratePkcePair(200);
    EXPECT_GE(pkce.verifier.size(), 43u);
    EXPECT_LE(pkce.verifier.size(), 128u);
    EXPECT_FALSE(pkce.challenge.empty());
}

TEST(OAuthTest, BuildAuthorizationUrlIncludesParameters) {
    AuthorizationUrlRequest request{};
    request.authorize_endpoint = "https://app.alpaca.markets/oauth/authorize";
    request.client_id = "client";
    request.redirect_uri = "https://example.com/callback";
    request.code_challenge = "challenge";
    request.scope = "account trading";
    request.state = "state123";
    request.prompt = "consent";
    request.broker_account_id = "A1";

    std::string url = alpaca::BuildAuthorizationUrl(request);
    EXPECT_NE(url.find("client_id=client"), std::string::npos);
    EXPECT_NE(url.find("redirect_uri=https%3A%2F%2Fexample.com%2Fcallback"), std::string::npos);
    EXPECT_NE(url.find("code_challenge=challenge"), std::string::npos);
    EXPECT_NE(url.find("code_challenge_method=S256"), std::string::npos);
    EXPECT_NE(url.find("scope=account%20trading"), std::string::npos);
    EXPECT_NE(url.find("state=state123"), std::string::npos);
    EXPECT_NE(url.find("prompt=consent"), std::string::npos);
    EXPECT_NE(url.find("broker_account_id=A1"), std::string::npos);
}

TEST(OAuthTest, ExchangeAuthorizationCodeBuildsFormRequest) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(alpaca::HttpResponse{200,
                                             R"({"access_token":"token","refresh_token":"refresh","expires_in":3600,"scope":"account"})",
                                             {}});

    OAuthClient client{"https://broker-api.sandbox.alpaca.markets/oauth/token", fake};

    AuthorizationCodeTokenRequest request{};
    request.client_id = "client";
    request.redirect_uri = "https://example.com/callback";
    request.code = "auth-code";
    request.code_verifier = "verifier";

    OAuthTokenResponse tokens = client.ExchangeAuthorizationCode(request);

    ASSERT_EQ(fake->requests().size(), 1u);
    auto const& recorded = fake->requests().front().request;
    EXPECT_EQ(recorded.method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded.url, "https://broker-api.sandbox.alpaca.markets/oauth/token");
    EXPECT_EQ(recorded.headers.at("Content-Type"), "application/x-www-form-urlencoded");
    EXPECT_NE(recorded.headers.find("Accept"), recorded.headers.end());
    EXPECT_NE(recorded.body.find("grant_type=authorization_code"), std::string::npos);
    EXPECT_NE(recorded.body.find("client_id=client"), std::string::npos);
    EXPECT_NE(recorded.body.find("redirect_uri=https%3A%2F%2Fexample.com%2Fcallback"), std::string::npos);
    EXPECT_NE(recorded.body.find("code=auth-code"), std::string::npos);
    EXPECT_NE(recorded.body.find("code_verifier=verifier"), std::string::npos);

    EXPECT_EQ(tokens.access_token, "token");
    ASSERT_TRUE(tokens.refresh_token.has_value());
    EXPECT_EQ(*tokens.refresh_token, "refresh");
    ASSERT_TRUE(tokens.expires_in.has_value());
    EXPECT_EQ(tokens.expires_in->count(), 3600);
    ASSERT_TRUE(tokens.scope.has_value());
    EXPECT_EQ(*tokens.scope, "account");
}

TEST(OAuthTest, RefreshAccessTokenBuildsFormRequest) {
    auto fake = std::make_shared<FakeHttpClient>();
    fake->push_response(alpaca::HttpResponse{200, R"({"access_token":"token","expires_in":1800})", {}});

    OAuthClient client{"https://broker-api.sandbox.alpaca.markets/oauth/token", fake};

    RefreshTokenRequest request{};
    request.client_id = "client";
    request.refresh_token = "refresh";

    OAuthTokenResponse tokens = client.RefreshAccessToken(request);

    ASSERT_EQ(fake->requests().size(), 1u);
    auto const& recorded = fake->requests().front().request;
    EXPECT_NE(recorded.body.find("grant_type=refresh_token"), std::string::npos);
    EXPECT_NE(recorded.body.find("refresh_token=refresh"), std::string::npos);

    EXPECT_EQ(tokens.access_token, "token");
    ASSERT_TRUE(tokens.expires_in.has_value());
    EXPECT_EQ(tokens.expires_in->count(), 1800);
}

TEST(OAuthTest, ApplySetsBearerToken) {
    alpaca::Configuration config;
    config.api_key_id = "key";
    config.api_secret_key = "secret";

    OAuthTokenResponse tokens{};
    tokens.access_token = "bearer";
    tokens.apply(config);

    EXPECT_TRUE(config.api_key_id.empty());
    EXPECT_TRUE(config.api_secret_key.empty());
    ASSERT_TRUE(config.bearer_token.has_value());
    EXPECT_EQ(*config.bearer_token, "bearer");
}

