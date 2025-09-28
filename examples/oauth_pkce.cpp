#include "alpaca/Configuration.hpp"
#include "alpaca/HttpClientFactory.hpp"
#include "alpaca/OAuth.hpp"

#include <iostream>

int main() {
    // Generate a PKCE verifier/challenge pair before redirecting the user.
    alpaca::PkcePair pkce = alpaca::GeneratePkcePair();

    alpaca::AuthorizationUrlRequest url_request{};
    url_request.authorize_endpoint = "https://app.alpaca.markets/oauth/authorize";
    url_request.client_id = "YOUR_CLIENT_ID";
    url_request.redirect_uri = "https://example.com/callback";
    url_request.code_challenge = pkce.challenge;
    url_request.scope = "account trading";
    url_request.state = "opaque-csrf-token";

    std::cout << "Open this URL in a browser to authorize the application:\n"
              << alpaca::BuildAuthorizationUrl(url_request) << "\n\n";

    std::cout << "Enter the authorization code returned to your redirect URI: ";
    std::string authorization_code;
    std::getline(std::cin, authorization_code);

    auto http_client = alpaca::create_default_http_client({});
    alpaca::OAuthClient oauth_client{"https://broker-api.alpaca.markets/oauth/token", http_client};

    alpaca::AuthorizationCodeTokenRequest token_request{};
    token_request.client_id = "YOUR_CLIENT_ID";
    token_request.redirect_uri = "https://example.com/callback";
    token_request.code = authorization_code;
    token_request.code_verifier = pkce.verifier;

    alpaca::OAuthTokenResponse tokens = oauth_client.ExchangeAuthorizationCode(token_request);

    alpaca::Configuration configuration;
    configuration.broker_base_url = "https://broker-api.alpaca.markets";
    tokens.apply(configuration);

    std::cout << "Received access token (" << tokens.token_type << ") with scope "
              << (tokens.scope.has_value() ? *tokens.scope : std::string{"<none>"}) << "\n";
}
