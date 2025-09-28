#include "alpaca/Configuration.hpp"

#include <cstdlib>
#include <utility>

#include "alpaca/Environments.hpp"

namespace {

std::string env_or(char const *name, std::string value) {
    if (auto const *env = std::getenv(name); env != nullptr) {
        return env;
    }
    return value;
}

} // namespace

namespace alpaca {

Configuration Configuration::FromEnvironment(Environment const& environment, std::string api_key_id,
                                             std::string api_secret_key) {
    Configuration cfg;
    cfg.api_key_id = env_or("APCA_API_KEY_ID", std::move(api_key_id));
    cfg.api_secret_key = env_or("APCA_API_SECRET_KEY", std::move(api_secret_key));
    cfg.trading_base_url = env_or("APCA_API_BASE_URL", environment.trading_base_url);
    cfg.data_base_url = env_or("APCA_API_DATA_URL", environment.data_base_url);
    cfg.broker_base_url = env_or("APCA_API_BROKER_URL", environment.broker_base_url);
    cfg.trading_stream_url = env_or("APCA_API_STREAM_URL", environment.trading_stream_url);
    cfg.market_data_stream_url = env_or("APCA_API_DATA_STREAM_URL", environment.market_data_stream_url);
    cfg.crypto_stream_url = env_or("APCA_API_CRYPTO_STREAM_URL", environment.crypto_stream_url);
    cfg.options_stream_url = env_or("APCA_API_OPTIONS_STREAM_URL", environment.options_stream_url);
    return cfg;
}

Configuration Configuration::Live(std::string api_key_id, std::string api_secret_key) {
    return FromEnvironment(Environments::Live(), std::move(api_key_id), std::move(api_secret_key));
}

Configuration Configuration::Paper(std::string api_key_id, std::string api_secret_key) {
    return FromEnvironment(Environments::Paper(), std::move(api_key_id), std::move(api_secret_key));
}

} // namespace alpaca
