#pragma once

#include <string>

namespace alpaca {

/// Describes the base URLs for REST and streaming services within an Alpaca environment.
struct Environment {
    std::string name;
    std::string trading_base_url;
    std::string data_base_url;
    std::string broker_base_url;
    std::string trading_stream_url;
    std::string market_data_stream_url;
    std::string crypto_stream_url;
    std::string options_stream_url;
};

/// Factory helpers that mirror the official SDK environment catalog.
class Environments {
  public:
    /// Returns the Alpaca paper trading environment endpoints.
    static Environment Paper();

    /// Returns the Alpaca live trading environment endpoints.
    static Environment Live();
};

} // namespace alpaca
