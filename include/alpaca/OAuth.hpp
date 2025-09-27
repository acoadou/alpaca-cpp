#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "alpaca/Configuration.hpp"
#include "alpaca/HttpClient.hpp"
#include "alpaca/HttpHeaders.hpp"

namespace alpaca {

/// Represents a PKCE verifier/challenge pair.
struct PkcePair {
    std::string verifier;
    std::string challenge;
};

/// Generates a PKCE verifier and its SHA-256 based challenge.
///
/// The verifier length will be clamped to the range [43, 128] as mandated by the
/// specification.
PkcePair GeneratePkcePair(std::size_t verifier_length = 64);

/// Parameters used to construct the OAuth authorization URL.
struct AuthorizationUrlRequest {
    std::string authorize_endpoint;
    std::string client_id;
    std::string redirect_uri;
    std::string code_challenge;
    std::optional<std::string> response_type{"code"};
    std::optional<std::string> scope{};
    std::optional<std::string> state{};
    std::optional<std::string> prompt{};
    std::optional<std::string> broker_account_id{};
    std::vector<std::pair<std::string, std::string>> extra_query_params{};
};

/// Builds a user-facing authorization URL for Connect style OAuth flows.
std::string BuildAuthorizationUrl(AuthorizationUrlRequest const& request);

/// Represents the decoded token payload returned by the OAuth token endpoint.
struct OAuthTokenResponse {
    std::string access_token;
    std::string token_type{"Bearer"};
    std::optional<std::string> refresh_token{};
    std::optional<std::chrono::seconds> expires_in{};
    std::optional<std::chrono::system_clock::time_point> expires_at{};
    std::optional<std::string> scope{};

    /// Applies the access token to an Alpaca configuration object.
    void apply(Configuration& configuration) const;
};

/// Request payload for exchanging an authorization code for tokens.
struct AuthorizationCodeTokenRequest {
    std::string client_id;
    std::string redirect_uri;
    std::string code;
    std::string code_verifier;
    std::optional<std::string> client_secret{};
};

/// Request payload for refreshing an access token.
struct RefreshTokenRequest {
    std::string client_id;
    std::string refresh_token;
    std::optional<std::string> client_secret{};
};

/// Lightweight OAuth 2.0 client for interacting with Alpaca's Connect token endpoint.
class OAuthClient {
  public:
    struct Options {
        std::chrono::milliseconds timeout{std::chrono::seconds{30}};
        bool verify_ssl{true};
        bool verify_hostname{true};
        std::string ca_bundle_path{};
        std::string ca_bundle_dir{};
        HttpHeaders default_headers{};
    };

    OAuthClient(std::string token_endpoint, HttpClientPtr http_client);
    OAuthClient(std::string token_endpoint, HttpClientPtr http_client, Options options);

    [[nodiscard]] OAuthTokenResponse ExchangeAuthorizationCode(AuthorizationCodeTokenRequest const& request) const;

    [[nodiscard]] OAuthTokenResponse RefreshAccessToken(RefreshTokenRequest const& request) const;

  private:
    [[nodiscard]] OAuthTokenResponse parse_token_response(std::string const& body) const;

    HttpResponse post_form(std::vector<std::pair<std::string, std::string>> const& params,
                           HttpHeaders extra_headers = {}) const;

    std::string token_endpoint_;
    HttpClientPtr http_client_;
    Options options_;
};

} // namespace alpaca

