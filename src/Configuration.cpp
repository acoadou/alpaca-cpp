#include "alpaca/Configuration.hpp"

#include <utility>

#include "alpaca/Environments.hpp"

namespace alpaca {

Configuration Configuration::FromEnvironment(Environment const& environment, std::string api_key_id,
                                             std::string api_secret_key) {
    Configuration cfg;
    cfg.api_key_id = std::move(api_key_id);
    cfg.api_secret_key = std::move(api_secret_key);
    cfg.trading_base_url = environment.trading_base_url;
    cfg.data_base_url = environment.data_base_url;
    cfg.broker_base_url = environment.broker_base_url;
    cfg.trading_stream_url = environment.trading_stream_url;
    cfg.market_data_stream_url = environment.market_data_stream_url;
    cfg.crypto_stream_url = environment.crypto_stream_url;
    cfg.options_stream_url = environment.options_stream_url;
    return cfg;
}

Configuration Configuration::Live(std::string api_key_id, std::string api_secret_key) {
    return FromEnvironment(Environments::Live(), std::move(api_key_id), std::move(api_secret_key));
}

Configuration Configuration::Paper(std::string api_key_id, std::string api_secret_key) {
    return FromEnvironment(Environments::Paper(), std::move(api_key_id), std::move(api_secret_key));
}

} // namespace alpaca
