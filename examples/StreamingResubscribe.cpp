#include <alpaca/Configuration.hpp>
#include <alpaca/Environments.hpp>
#include <alpaca/Streaming.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <variant>

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

    alpaca::streaming::WebSocketClient socket(
        config.trading_stream_url,
        config.api_key_id,
        config.api_secret_key,
        alpaca::streaming::StreamFeed::Trading);

    socket.set_reconnect_policy({
        std::chrono::milliseconds{250},
        std::chrono::seconds{15},
        2.0,
        std::chrono::milliseconds{250}});
    socket.set_ping_interval(std::chrono::seconds{15});

    socket.set_message_handler([](alpaca::streaming::StreamMessage const& message,
                                  alpaca::streaming::MessageCategory category) {
        switch (category) {
        case alpaca::streaming::MessageCategory::OrderUpdate: {
            auto const& update = std::get<alpaca::streaming::OrderUpdateMessage>(message);
            std::cout << "[order] " << update.event << " -> " << update.order.id << std::endl;
            break;
        }
        case alpaca::streaming::MessageCategory::AccountUpdate: {
            std::cout << "[account] updated available balance" << std::endl;
            break;
        }
        case alpaca::streaming::MessageCategory::Error: {
            auto const& error = std::get<alpaca::streaming::ErrorMessage>(message);
            std::cerr << "[error] " << error.message << std::endl;
            break;
        }
        case alpaca::streaming::MessageCategory::Control: {
            auto const& control = std::get<alpaca::streaming::ControlMessage>(message);
            std::cout << "[control] " << control.type << std::endl;
            break;
        }
        default:
            break;
        }
    });

    socket.set_open_handler([&socket]() {
        std::cout << "Websocket connection opened, subscribing to trading streams..." << std::endl;
        socket.listen({"trade_updates", "account_updates"});
    });

    socket.set_close_handler([]() {
        std::cout << "Connection closed, attempting automatic reconnect..." << std::endl;
    });

    socket.set_error_handler([](std::string const& message) {
        std::cerr << "Websocket error: " << message << std::endl;
    });

    socket.connect();

    std::cout << "Streaming in progress. Press Enter to exit." << std::endl;
    std::string line;
    std::getline(std::cin, line);

    socket.disconnect();
    return 0;
}
