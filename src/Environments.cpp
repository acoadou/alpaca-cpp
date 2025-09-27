#include "alpaca/Environments.hpp"

#include <utility>

namespace alpaca {
namespace {
Environment make_environment(std::string name, std::string trading_base, std::string data_base, std::string broker_base,
                             std::string trading_stream, std::string market_data_stream, std::string crypto_stream,
                             std::string options_stream) {
    return Environment{std::move(name),          std::move(trading_base),   std::move(data_base),
                       std::move(broker_base),   std::move(trading_stream), std::move(market_data_stream),
                       std::move(crypto_stream), std::move(options_stream)};
}
} // namespace

Environment Environments::Paper() {
    return make_environment("paper", "https://paper-api.alpaca.markets", "https://data.alpaca.markets/v2",
                            "https://broker-api.sandbox.alpaca.markets", "wss://paper-api.alpaca.markets/stream",
                            "wss://stream.data.alpaca.markets/v2", "wss://stream.data.alpaca.markets/v1beta1/crypto",
                            "wss://stream.data.alpaca.markets/v2/options");
}

Environment Environments::Live() {
    return make_environment("live", "https://api.alpaca.markets", "https://data.alpaca.markets/v2",
                            "https://broker-api.alpaca.markets", "wss://api.alpaca.markets/stream",
                            "wss://stream.data.alpaca.markets/v2", "wss://stream.data.alpaca.markets/v1beta1/crypto",
                            "wss://stream.data.alpaca.markets/v2/options");
}

} // namespace alpaca
