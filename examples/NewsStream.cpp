#include <alpaca/Configuration.hpp>
#include <alpaca/Environments.hpp>
#include <alpaca/Streaming.hpp>

#include <iostream>
#include <string>
#include <variant>

int main() {
    auto config = alpaca::Configuration::FromEnvironment(alpaca::Environments::Paper(), "", "");
    if (!config.has_credentials()) {
        std::cerr << "Please set APCA_API_KEY_ID and APCA_API_SECRET_KEY in the environment." << std::endl;
        return 1;
    }

    alpaca::streaming::WebSocketClient socket(config.market_data_stream_url, config.api_key_id, config.api_secret_key,
                                              alpaca::streaming::StreamFeed::MarketData);

    socket.set_message_handler(
    [](alpaca::streaming::StreamMessage const& message, alpaca::streaming::MessageCategory category) {
        if (category != alpaca::streaming::MessageCategory::News) {
            return;
        }

        auto const& article = std::get<alpaca::streaming::NewsMessage>(message);
        std::cout << "[news]";
        for (auto const& symbol : article.symbols) {
            std::cout << ' ' << symbol;
        }
        std::cout << " :: " << article.headline << std::endl;
        if (article.summary) {
            std::cout << "        " << *article.summary << std::endl;
        }
    });

    socket.set_open_handler([&socket]() {
        std::cout << "Connected, subscribing to the Alpaca news stream..." << std::endl;
        alpaca::streaming::MarketSubscription subscription;
        subscription.news = {"*"};
        socket.subscribe(subscription);
    });

    socket.set_error_handler([](std::string const& message) {
        std::cerr << "Websocket error: " << message << std::endl;
    });

    socket.set_close_handler([]() {
        std::cout << "Connection closed." << std::endl;
    });

    socket.connect();

    std::cout << "Press Enter to stop streaming." << std::endl;
    std::string line;
    std::getline(std::cin, line);

    socket.disconnect();
    return 0;
}
