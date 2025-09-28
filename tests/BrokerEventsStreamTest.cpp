#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "alpaca/Exceptions.hpp"
#include "alpaca/Streaming.hpp"

using namespace std::chrono_literals;

namespace alpaca::streaming {
namespace {

class ScriptedTransport : public detail::BrokerEventsTransport {
  public:
    struct Script {
        std::vector<std::string> chunks;
        bool throw_error{false};
    };

    ScriptedTransport(detail::BrokerEventsTransport::Parameters params, Script script,
                      std::vector<detail::BrokerEventsTransport::Parameters>* log)
      : params_(std::move(params)), script_(std::move(script)), log_(log) {
    }

    void run(DataCallback on_data, CloseCallback on_close) override {
        if (log_ != nullptr) {
            log_->push_back(params_);
        }
        for (auto const& chunk : script_.chunks) {
            if (stop_requested_.load()) {
                break;
            }
            on_data(chunk);
        }
        if (script_.throw_error && !stop_requested_.load()) {
            throw alpaca::StreamingException(alpaca::ErrorCode::Unknown, "scripted failure");
        }
        if (!stop_requested_.load() && !script_.throw_error) {
            on_close();
        }
    }

    void stop() override {
        stop_requested_.store(true);
    }

  private:
    detail::BrokerEventsTransport::Parameters params_{};
    Script script_{};
    std::vector<detail::BrokerEventsTransport::Parameters>* log_{nullptr};
    std::atomic<bool> stop_requested_{false};
};

class ScriptedFactory {
  public:
    explicit ScriptedFactory(std::vector<ScriptedTransport::Script> scripts) : scripts_(std::move(scripts)) {
    }

    std::unique_ptr<detail::BrokerEventsTransport> operator()(detail::BrokerEventsTransport::Parameters const& params) {
        if (index_ >= scripts_.size()) {
            return nullptr;
        }
        return std::make_unique<ScriptedTransport>(params, scripts_[index_++], &parameters_);
    }

    std::vector<detail::BrokerEventsTransport::Parameters> const& parameters() const {
        return parameters_;
    }

  private:
    std::vector<ScriptedTransport::Script> scripts_{};
    std::size_t index_{0};
    std::vector<detail::BrokerEventsTransport::Parameters> parameters_{};
};

TEST(BrokerEventsStreamTest, ReconnectsAndDispatchesEvents) {
    Configuration config;
    config.api_key_id = "key";
    config.api_secret_key = "secret";
    config.broker_base_url = "https://example.com";

    BrokerEventsStreamOptions options;
    options.resource = "accounts";
    options.reconnect.initial_delay = std::chrono::milliseconds{1};
    options.reconnect.max_delay = std::chrono::milliseconds{1};
    options.reconnect.jitter = std::chrono::milliseconds{0};
    options.reconnect.multiplier = 1.0;

    ScriptedFactory factory(
    {ScriptedTransport::Script{
         {"id: 1\n",
          "data: "
          "{\"id\":\"evt-1\",\"type\":\"account\",\"account_id\":\"A1\",\"created_at\":\"2024-01-01T00:00:00Z\"}\n\n"}},
     ScriptedTransport::Script{{"id: 2\n", "data: "
                                           "{\"id\":\"evt-2\",\"type\":\"account\",\"account_id\":\"A1\",\"created_"
                                           "at\":\"2024-01-01T00:00:01Z\"}\n\n"}}});

    BrokerEventsStream stream(config, options);
    stream.set_transport_factory_for_testing([&factory](detail::BrokerEventsTransport::Parameters const& params) {
        return factory(params);
    });

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<BrokerEvent> received;
    bool done = false;

    stream.set_event_handler([&](BrokerEvent const& event) {
        std::lock_guard<std::mutex> lock(mutex);
        received.push_back(event);
        if (received.size() >= 2U) {
            done = true;
        }
        cv.notify_all();
    });

    std::vector<std::string> errors;
    stream.set_error_handler([&](std::exception_ptr ptr) {
        try {
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        } catch (std::exception const& ex) {
            std::lock_guard<std::mutex> lock(mutex);
            errors.emplace_back(ex.what());
            cv.notify_all();
        }
    });

    stream.start();

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait_for(lock, 1s, [&]() {
            return done;
        });
    }

    stream.stop();

    EXPECT_THAT(errors, ::testing::IsEmpty());
    ASSERT_EQ(received.size(), 2U);
    EXPECT_EQ(received[0].id, "evt-1");
    EXPECT_EQ(received[1].id, "evt-2");

    auto last_id = stream.last_event_id();
    ASSERT_TRUE(last_id.has_value());
    EXPECT_EQ(*last_id, "2");

    auto const& attempts = factory.parameters();
    ASSERT_GE(attempts.size(), 2U);
    EXPECT_EQ(attempts.front().url, "https://example.com/v2/events/accounts");
    EXPECT_FALSE(attempts.front().headers.get("Last-Event-ID").has_value());
    ASSERT_TRUE(attempts.at(1).headers.get("Last-Event-ID").has_value());
    EXPECT_EQ(*attempts.at(1).headers.get("Last-Event-ID"), "1");
}

TEST(BrokerEventsStreamTest, ReportsTransportErrors) {
    Configuration config;
    config.api_key_id = "key";
    config.api_secret_key = "secret";

    BrokerEventsStreamOptions options;
    options.resource = "accounts";
    options.reconnect.initial_delay = std::chrono::milliseconds{1};
    options.reconnect.max_delay = std::chrono::milliseconds{1};
    options.reconnect.jitter = std::chrono::milliseconds{0};
    options.reconnect.multiplier = 1.0;

    ScriptedFactory factory({
        ScriptedTransport::Script{{}, true},
        ScriptedTransport::Script{{"id: 7\n", "data: "
                                              "{\"id\":\"evt-7\",\"type\":\"account\",\"account_id\":\"A7\",\"created_"
                                              "at\":\"2024-01-01T00:00:07Z\"}\n\n"}}
    });

    BrokerEventsStream stream(config, options);
    stream.set_transport_factory_for_testing([&factory](detail::BrokerEventsTransport::Parameters const& params) {
        return factory(params);
    });

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<BrokerEvent> received;
    std::vector<std::string> errors;

    stream.set_event_handler([&](BrokerEvent const& event) {
        std::lock_guard<std::mutex> lock(mutex);
        received.push_back(event);
        cv.notify_all();
    });

    stream.set_error_handler([&](std::exception_ptr ptr) {
        try {
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        } catch (std::exception const& ex) {
            std::lock_guard<std::mutex> lock(mutex);
            errors.emplace_back(ex.what());
            cv.notify_all();
        }
    });

    stream.start();

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait_for(lock, 1s, [&]() {
            return received.size() >= 1U && !errors.empty();
        });
    }

    stream.stop();

    ASSERT_FALSE(errors.empty());
    EXPECT_THAT(errors.front(), ::testing::HasSubstr("scripted failure"));
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received.front().id, "evt-7");
}

} // namespace
} // namespace alpaca::streaming
