#include "alpaca/OAuth.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <openssl/sha.h>

#include "alpaca/ApiException.hpp"
#include "alpaca/Json.hpp"

namespace alpaca {
namespace {
constexpr std::string_view kCodeVerifierAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";

bool is_unreserved(unsigned char c) {
    return std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.' || c == '~';
}

std::string url_encode(std::string_view value) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex << std::uppercase;
    for (unsigned char c : value) {
        if (is_unreserved(c)) {
            encoded << static_cast<char>(c);
        } else {
            encoded << '%' << std::setw(2) << static_cast<int>(c);
        }
    }
    return encoded.str();
}

std::string base64_url_encode(std::span<unsigned char const> data) {
    static constexpr char const table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 3 <= data.size()) {
        unsigned int const chunk = (static_cast<unsigned int>(data[i]) << 16) |
                                   (static_cast<unsigned int>(data[i + 1]) << 8) |
                                   static_cast<unsigned int>(data[i + 2]);
        encoded.push_back(table[(chunk >> 18) & 0x3F]);
        encoded.push_back(table[(chunk >> 12) & 0x3F]);
        encoded.push_back(table[(chunk >> 6) & 0x3F]);
        encoded.push_back(table[chunk & 0x3F]);
        i += 3;
    }

    std::size_t remainder = data.size() - i;
    if (remainder == 1) {
        unsigned char const b0 = data[i];
        encoded.push_back(table[(b0 & 0xFC) >> 2]);
        encoded.push_back(table[(b0 & 0x03) << 4]);
    } else if (remainder == 2) {
        unsigned char const b0 = data[i];
        unsigned char const b1 = data[i + 1];
        encoded.push_back(table[(b0 & 0xFC) >> 2]);
        encoded.push_back(table[((b0 & 0x03) << 4) | ((b1 & 0xF0) >> 4)]);
        encoded.push_back(table[(b1 & 0x0F) << 2]);
    }

    return encoded;
}

std::string build_query(std::vector<std::pair<std::string, std::string>> const& params) {
    std::ostringstream query;
    bool first = true;
    for (auto const& [key, value] : params) {
        if (!first) {
            query << '&';
        }
        first = false;
        query << url_encode(key) << '=' << url_encode(value);
    }
    return query.str();
}

Json parse_body(std::string const& body) {
    try {
        return Json::parse(body);
    } catch (Json::parse_error const&) {
        throw ApiException(500, "Unable to parse OAuth token response", body, {});
    }
}

} // namespace

PkcePair GeneratePkcePair(std::size_t verifier_length) {
    verifier_length = std::clamp<std::size_t>(verifier_length, 43, 128);

    std::random_device rd;
    std::uniform_int_distribution<std::size_t> dist(0, kCodeVerifierAlphabet.size() - 1);

    std::string verifier;
    verifier.reserve(verifier_length);
    for (std::size_t i = 0; i < verifier_length; ++i) {
        verifier.push_back(kCodeVerifierAlphabet[dist(rd)]);
    }

    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    SHA256(reinterpret_cast<unsigned char const*>(verifier.data()), verifier.size(), digest.data());
    std::string challenge = base64_url_encode(std::span<unsigned char const>{digest.data(), digest.size()});

    return PkcePair{std::move(verifier), std::move(challenge)};
}

std::string BuildAuthorizationUrl(AuthorizationUrlRequest const& request) {
    if (request.authorize_endpoint.empty()) {
        throw std::invalid_argument("authorize_endpoint must not be empty");
    }
    if (request.client_id.empty()) {
        throw std::invalid_argument("client_id must not be empty");
    }
    if (request.redirect_uri.empty()) {
        throw std::invalid_argument("redirect_uri must not be empty");
    }
    if (request.code_challenge.empty()) {
        throw std::invalid_argument("code_challenge must not be empty");
    }

    std::vector<std::pair<std::string, std::string>> params;
    params.emplace_back("client_id", request.client_id);
    params.emplace_back("redirect_uri", request.redirect_uri);
    params.emplace_back("code_challenge", request.code_challenge);
    params.emplace_back("code_challenge_method", "S256");

    if (request.response_type.has_value()) {
        params.emplace_back("response_type", *request.response_type);
    }
    if (request.scope.has_value()) {
        params.emplace_back("scope", *request.scope);
    }
    if (request.state.has_value()) {
        params.emplace_back("state", *request.state);
    }
    if (request.prompt.has_value()) {
        params.emplace_back("prompt", *request.prompt);
    }
    if (request.broker_account_id.has_value()) {
        params.emplace_back("broker_account_id", *request.broker_account_id);
    }

    for (auto const& extra : request.extra_query_params) {
        params.push_back(extra);
    }

    std::string query = build_query(params);

    std::ostringstream url;
    url << request.authorize_endpoint;
    if (!query.empty()) {
        char separator = request.authorize_endpoint.find('?') == std::string::npos ? '?' : '&';
        url << separator << query;
    }
    return url.str();
}

void OAuthTokenResponse::apply(Configuration& configuration) const {
    configuration.api_key_id.clear();
    configuration.api_secret_key.clear();
    configuration.bearer_token = access_token;
}

OAuthClient::OAuthClient(std::string token_endpoint, HttpClientPtr http_client)
  : OAuthClient(std::move(token_endpoint), std::move(http_client), Options{}) {}

OAuthClient::OAuthClient(std::string token_endpoint, HttpClientPtr http_client, Options options)
  : token_endpoint_(std::move(token_endpoint)), http_client_(std::move(http_client)), options_(std::move(options)) {
    if (!http_client_) {
        throw std::invalid_argument("OAuthClient requires a non-null HttpClient");
    }
    if (token_endpoint_.empty()) {
        throw std::invalid_argument("token_endpoint must not be empty");
    }
}

OAuthTokenResponse OAuthClient::ExchangeAuthorizationCode(AuthorizationCodeTokenRequest const& request) const {
    if (request.client_id.empty()) {
        throw std::invalid_argument("client_id must not be empty");
    }
    if (request.redirect_uri.empty()) {
        throw std::invalid_argument("redirect_uri must not be empty");
    }
    if (request.code.empty()) {
        throw std::invalid_argument("code must not be empty");
    }
    if (request.code_verifier.empty()) {
        throw std::invalid_argument("code_verifier must not be empty");
    }

    std::vector<std::pair<std::string, std::string>> params{{"grant_type", "authorization_code"},
                                                            {"client_id", request.client_id},
                                                            {"redirect_uri", request.redirect_uri},
                                                            {"code", request.code},
                                                            {"code_verifier", request.code_verifier}};
    if (request.client_secret.has_value()) {
        params.emplace_back("client_secret", *request.client_secret);
    }

    HttpResponse response = post_form(params);
    if (response.status_code >= 400) {
        Json error_body;
        std::string message = "HTTP " + std::to_string(response.status_code);
        try {
            error_body = Json::parse(response.body);
            if (error_body.contains("error_description")) {
                message = error_body.at("error_description").get<std::string>();
            } else if (error_body.contains("error")) {
                message = error_body.at("error").get<std::string>();
            }
        } catch (std::exception const&) {
            // Keep default message
        }
        throw ApiException(response.status_code, message, response.body, response.headers);
    }

    return parse_token_response(response.body);
}

OAuthTokenResponse OAuthClient::RefreshAccessToken(RefreshTokenRequest const& request) const {
    if (request.client_id.empty()) {
        throw std::invalid_argument("client_id must not be empty");
    }
    if (request.refresh_token.empty()) {
        throw std::invalid_argument("refresh_token must not be empty");
    }

    std::vector<std::pair<std::string, std::string>> params{{"grant_type", "refresh_token"},
                                                            {"client_id", request.client_id},
                                                            {"refresh_token", request.refresh_token}};
    if (request.client_secret.has_value()) {
        params.emplace_back("client_secret", *request.client_secret);
    }

    HttpResponse response = post_form(params);
    if (response.status_code >= 400) {
        Json error_body;
        std::string message = "HTTP " + std::to_string(response.status_code);
        try {
            error_body = Json::parse(response.body);
            if (error_body.contains("error_description")) {
                message = error_body.at("error_description").get<std::string>();
            } else if (error_body.contains("error")) {
                message = error_body.at("error").get<std::string>();
            }
        } catch (std::exception const&) {
            // Keep default message
        }
        throw ApiException(response.status_code, message, response.body, response.headers);
    }

    return parse_token_response(response.body);
}

OAuthTokenResponse OAuthClient::parse_token_response(std::string const& body) const {
    Json payload = parse_body(body);

    if (!payload.contains("access_token")) {
        throw ApiException(500, "OAuth response missing access_token", body, {});
    }

    OAuthTokenResponse token{};
    token.access_token = payload.at("access_token").get<std::string>();
    if (payload.contains("token_type") && !payload.at("token_type").is_null()) {
        token.token_type = payload.at("token_type").get<std::string>();
    }
    if (payload.contains("refresh_token") && !payload.at("refresh_token").is_null()) {
        token.refresh_token = payload.at("refresh_token").get<std::string>();
    }
    if (payload.contains("expires_in") && !payload.at("expires_in").is_null()) {
        long long seconds = payload.at("expires_in").get<long long>();
        if (seconds > 0) {
            token.expires_in = std::chrono::seconds{seconds};
            token.expires_at = std::chrono::system_clock::now() + *token.expires_in;
        }
    }
    if (payload.contains("scope") && !payload.at("scope").is_null()) {
        token.scope = payload.at("scope").get<std::string>();
    }
    return token;
}

HttpResponse OAuthClient::post_form(std::vector<std::pair<std::string, std::string>> const& params,
                                    HttpHeaders extra_headers) const {
    HttpRequest request;
    request.method = HttpMethod::POST;
    request.url = token_endpoint_;
    request.timeout = options_.timeout;
    request.headers = options_.default_headers;
    for (auto const& header : extra_headers) {
        request.headers[header.first] = header.second;
    }
    request.headers["Accept"] = "application/json";
    request.headers["Content-Type"] = "application/x-www-form-urlencoded";
    request.verify_peer = options_.verify_ssl;
    request.verify_host = options_.verify_hostname;
    request.ca_bundle_path = options_.ca_bundle_path;
    request.ca_bundle_dir = options_.ca_bundle_dir;

    request.body = build_query(params);

    return http_client_->send(request);
}

} // namespace alpaca

