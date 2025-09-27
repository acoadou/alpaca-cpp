#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

#include "alpaca/Environments.hpp"

namespace alpaca {

/// Represents configuration options for communicating with the Alpaca REST API.
struct Configuration {
    /// API key identifier assigned by Alpaca.
    std::string api_key_id;

    /// Secret API key assigned by Alpaca.
    std::string api_secret_key;

    /// Base URL for trading REST endpoints.
    std::string trading_base_url{"https://paper-api.alpaca.markets"};

    /// Base URL for market data REST endpoints.
    std::string data_base_url{"https://data.alpaca.markets/v2"};

    /// Base URL for broker REST endpoints.
    std::string broker_base_url{"https://broker-api.sandbox.alpaca.markets"};

    /// Base URL for trading streaming updates.
    std::string trading_stream_url{"wss://paper-api.alpaca.markets/stream"};

    /// Base URL for market data streaming feeds (stocks SIP/IEX).
    std::string market_data_stream_url{"wss://stream.data.alpaca.markets/v2"};

    /// Base URL for crypto streaming feeds.
    std::string crypto_stream_url{"wss://stream.data.alpaca.markets/v1beta1/crypto"};

    /// Base URL for options streaming feeds.
    std::string options_stream_url{"wss://stream.data.alpaca.markets/v2/options"};

    /// Timeout applied to outgoing HTTP requests.
    std::chrono::milliseconds timeout{std::chrono::seconds{30}};

    /// Additional headers to append to every HTTP request.
    std::unordered_map<std::string, std::string> default_headers{};

    /// Optional bearer token used for OAuth-style authentication.
    std::optional<std::string> bearer_token{};

    /// Enables TLS peer verification for HTTPS requests.
    bool verify_ssl{true};

    /// Enables HTTPS hostname verification.
    bool verify_hostname{true};

    /// Optional filesystem path to a bundle of trusted CA certificates.
    std::string ca_bundle_path{};

    /// Optional directory containing trusted CA certificates.
    std::string ca_bundle_dir{};

    /// Creates a configuration targeting the live trading environment.
    static Configuration Live(std::string api_key_id, std::string api_secret_key);

    /// Creates a configuration targeting the paper trading environment.
    static Configuration Paper(std::string api_key_id, std::string api_secret_key);

    /// Creates a configuration from an explicit environment descriptor.
    static Configuration FromEnvironment(Environment const& environment, std::string api_key_id,
                                         std::string api_secret_key);

    /// Returns \c true if the configuration contains the credentials required
    /// to authenticate.
    [[nodiscard]] bool has_credentials() const noexcept {
        if (!api_key_id.empty() && !api_secret_key.empty()) {
            return true;
        }
        if (bearer_token.has_value() && !bearer_token->empty()) {
            return true;
        }
        auto const authorization = default_headers.find("Authorization");
        return authorization != default_headers.end() && !authorization->second.empty();
    }
};

} // namespace alpaca
