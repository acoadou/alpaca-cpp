#include <alpaca/ApiException.hpp>
#include <alpaca/Chrono.hpp>
#include <alpaca/Configuration.hpp>
#include <alpaca/Environments.hpp>
#include <alpaca/MarketDataClient.hpp>
#include <alpaca/TradingClient.hpp>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace {
std::string format_price(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}
} // namespace

int main() {
    auto const* key = std::getenv("APCA_API_KEY_ID");
    auto const* secret = std::getenv("APCA_API_SECRET_KEY");
    if (key == nullptr || secret == nullptr) {
        std::cerr << "Please set APCA_API_KEY_ID and APCA_API_SECRET_KEY in the environment." << std::endl;
        return 1;
    }

    auto config = alpaca::Configuration::FromEnvironment(
        alpaca::Environments::Paper(),
        key,
        secret);

    alpaca::MarketDataClient market(config);
    alpaca::TradingClient trading(config);

    alpaca::StockBarsRequest bars_request;
    bars_request.timeframe = "1Min";
    bars_request.start = alpaca::since(std::chrono::hours{2});
    bars_request.limit = 50;

    double last_close = 0.0;
    try {
        for (auto const& bar : market.stock_bars_range("AAPL", bars_request)) {
            last_close = bar.close;
            std::cout << bar.timestamp << " close=" << bar.close << " volume=" << bar.volume << std::endl;
        }
    } catch (alpaca::ApiException const& ex) {
        std::cerr << "Failed to retrieve bars: " << ex.what() << std::endl;
        return 1;
    }

    if (last_close == 0.0) {
        std::cerr << "No market data received, aborting." << std::endl;
        return 1;
    }

    alpaca::NewOrderRequest order;
    order.symbol = "AAPL";
    order.side = alpaca::OrderSide::BUY;
    order.type = alpaca::OrderType::LIMIT;
    order.time_in_force = alpaca::TimeInForce::DAY;
    order.quantity = "1";
    order.limit_price = format_price(last_close - 0.10);

    bool submitted = false;
    int attempt = 0;
    while (!submitted) {
        try {
            ++attempt;
            auto placed = trading.submit_order(order);
            std::cout << "Order " << placed.id << " accepted at limit price " << *order.limit_price
                      << " after " << attempt << " attempt(s)." << std::endl;
            submitted = true;
        } catch (alpaca::ApiException const& ex) {
            std::cerr << "Attempt " << attempt << " failed (" << ex.status_code() << "): " << ex.what()
                      << std::endl;
            if (ex.status_code() == 429 || ex.status_code() == 503) {
                if (auto const delay = ex.retry_after()) {
                    std::cout << "Sleeping for " << delay->count() << " second(s) per Retry-After." << std::endl;
                    std::this_thread::sleep_for(*delay);
                } else {
                    std::cout << "Sleeping for default 1 second." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds{1});
                }
                continue;
            }
            return 1;
        }
    }

    return 0;
}
