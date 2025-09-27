#include "alpaca/Configuration.hpp"

namespace alpaca {

Configuration Configuration::Live(std::string api_key_id, std::string api_secret_key) {
    Configuration cfg;
    cfg.api_key_id = std::move(api_key_id);
    cfg.api_secret_key = std::move(api_secret_key);
    cfg.trading_base_url = "https://api.alpaca.markets";
    cfg.data_base_url = "https://data.alpaca.markets/v2";
    cfg.broker_base_url = "https://broker-api.alpaca.markets";
    return cfg;
}

Configuration Configuration::Paper(std::string api_key_id, std::string api_secret_key) {
    Configuration cfg;
    cfg.api_key_id = std::move(api_key_id);
    cfg.api_secret_key = std::move(api_secret_key);
    cfg.trading_base_url = "https://paper-api.alpaca.markets";
    cfg.data_base_url = "https://data.alpaca.markets/v2";
    cfg.broker_base_url = "https://broker-api.sandbox.alpaca.markets";
    return cfg;
}

} // namespace alpaca
