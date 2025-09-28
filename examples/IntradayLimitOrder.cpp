#include <alpaca/Chrono.hpp>
#include <alpaca/Configuration.hpp>
#include <alpaca/Environments.hpp>
#include <alpaca/Exceptions.hpp>
#include <alpaca/MarketDataClient.hpp>
#include <alpaca/Money.hpp>
#include <alpaca/TradingClient.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {
std::string format_price(alpaca::Money const& value) {
    return value.to_string();
}
} // namespace

int main() {
    auto config = alpaca::Configuration::FromEnvironment(alpaca::Environments::Paper(), "", "");
    if (!config.has_credentials()) {
        std::cerr << "Please set APCA_API_KEY_ID and APCA_API_SECRET_KEY in the environment." << std::endl;
        return 1;
    }

    alpaca::MarketDataClient market(config);
    alpaca::TradingClient trading(config);

    alpaca::StockBarsRequest bars_request;
    bars_request.timeframe = alpaca::TimeFrame::minute();
    bars_request.start = alpaca::since(std::chrono::hours{2});
    bars_request.limit = 50;

    alpaca::Money last_close{};
    try {
        for (auto const& bar : market.stock_bars_range("AAPL", bars_request)) {
            last_close = bar.close;
            std::cout << bar.timestamp << " close=" << bar.close << " volume=" << bar.volume << std::endl;
        }
    } catch (alpaca::Exception const& ex) {
        std::cerr << "Failed to retrieve bars: " << ex.what() << std::endl;
        return 1;
    }

    if (last_close == alpaca::Money{}) {
        std::cerr << "No market data received, aborting." << std::endl;
        return 1;
    }

    alpaca::NewOrderRequest order;
    order.symbol = "AAPL";
    order.side = alpaca::OrderSide::BUY;
    order.type = alpaca::OrderType::LIMIT;
    order.time_in_force = alpaca::TimeInForce::DAY;
    order.quantity = "1";
    order.limit_price = format_price(last_close - alpaca::Money{0.10});

    bool submitted = false;
    int attempt = 0;
    while (!submitted) {
        try {
            ++attempt;
            auto placed = trading.submit_order(order);
            std::cout << "Order " << placed.id << " accepted at limit price " << *order.limit_price << " after "
                      << attempt << " attempt(s)." << std::endl;
            submitted = true;
        } catch (alpaca::Exception const& ex) {
            std::cerr << "Attempt " << attempt << " failed (" << ex.status_code() << "): " << ex.what() << std::endl;
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
