#include "alpaca/Streaming.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "FakeHttpClient.hpp"
#include "alpaca/BackfillCoordinator.hpp"
#include "alpaca/Configuration.hpp"
#include "alpaca/Json.hpp"
#include "alpaca/MarketDataClient.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca::streaming {

class WebSocketClientHarness {
  public:
    static void feed(WebSocketClient& client, Json const& payload) {
        client.handle_payload(payload);
    }

    static std::size_t pending_message_count(WebSocketClient const& client) {
        std::lock_guard<std::mutex> lock(client.connection_mutex_);
        return client.pending_messages_.size();
    }

    static std::vector<Json> pending_messages(WebSocketClient const& client) {
        std::lock_guard<std::mutex> lock(client.connection_mutex_);
        return client.pending_messages_;
    }

    static void set_test_hooks(WebSocketClient& client, WebSocketClientTestHooks hooks) {
        client.test_hooks_ = std::move(hooks);
    }

    static void set_should_reconnect(WebSocketClient& client, bool value) {
        client.should_reconnect_.store(value);
    }

    static void set_manual_disconnect(WebSocketClient& client, bool value) {
        std::lock_guard<std::mutex> lock(client.connection_mutex_);
        client.manual_disconnect_ = value;
    }

    static void set_connected(WebSocketClient& client, bool value) {
        std::lock_guard<std::mutex> lock(client.connection_mutex_);
        client.connected_.store(value);
    }

    static void simulate_open(WebSocketClient& client) {
        {
            std::lock_guard<std::mutex> lock(client.connection_mutex_);
            client.connected_.store(true);
            client.should_reconnect_.store(true);
            client.manual_disconnect_ = false;
            client.reconnect_attempt_ = 0;
        }

        client.record_activity();
        client.authenticate();
        client.replay_subscriptions();

        std::vector<Json> pending;
        {
            std::lock_guard<std::mutex> lock(client.connection_mutex_);
            pending.swap(client.pending_messages_);
        }
        for (auto const& payload : pending) {
            set_connected(client, true);
            client.send_raw(payload);
        }

        if (client.open_handler_) {
            client.open_handler_();
        }
    }

    static void simulate_close(WebSocketClient& client) {
        bool should_retry = false;
        {
            std::lock_guard<std::mutex> lock(client.connection_mutex_);
            client.connected_.store(false);
            should_retry = client.should_reconnect_ && !client.manual_disconnect_;
        }
        if (client.close_handler_) {
            client.close_handler_();
        }
        if (should_retry) {
            client.schedule_reconnect();
        }
    }

    static void simulate_error(WebSocketClient& client, std::string reason) {
        bool should_retry = false;
        {
            std::lock_guard<std::mutex> lock(client.connection_mutex_);
            client.connected_.store(false);
            should_retry = client.should_reconnect_ && !client.manual_disconnect_;
        }
        if (client.error_handler_) {
            client.error_handler_(std::move(reason));
        }
        if (should_retry) {
            client.schedule_reconnect();
        }
    }

    static void drain_reconnect_thread(WebSocketClient& client) {
        std::thread thread;
        {
            std::lock_guard<std::mutex> lock(client.connection_mutex_);
            if (client.reconnect_thread_.joinable()) {
                thread = std::move(client.reconnect_thread_);
            }
        }
        if (thread.joinable()) {
            thread.join();
        }
    }

    static std::size_t reconnect_attempt(WebSocketClient const& client) {
        std::lock_guard<std::mutex> lock(client.connection_mutex_);
        return client.reconnect_attempt_;
    }

    static void set_last_message_time(WebSocketClient& client, std::int64_t value) {
        client.last_message_time_ns_.store(value);
    }

    static void handle_control(WebSocketClient& client, Json const& payload, std::string const& type) {
        client.handle_control_payload(payload, type);
    }

    static void trigger_heartbeat_timeout(WebSocketClient& client) {
        client.handle_heartbeat_timeout();
    }
};

} // namespace alpaca::streaming

namespace {

using alpaca::Json;
using alpaca::parse_timestamp;
using alpaca::streaming::MarketSubscription;
using alpaca::streaming::MessageCategory;
using alpaca::streaming::ReconnectPolicy;
using alpaca::streaming::StreamMessage;
using alpaca::streaming::WebSocketClient;
using alpaca::streaming::WebSocketClientHarness;
using alpaca::streaming::WebSocketClientTestHooks;

alpaca::HttpResponse MakeHttpResponse(std::string body) {
    return alpaca::HttpResponse{200, std::move(body), {}};
}

WebSocketClient make_client() {
    return WebSocketClient{"wss://example.com", "key", "secret"};
}

TEST(StreamingTest, RoutesUpdatedBarMessages) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::UpdatedBarMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::UpdatedBarMessage>(msg));
        message = std::get<alpaca::streaming::UpdatedBarMessage>(msg);
    });

    alpaca::Json payload{
        {"T",  "u"                             },
        {"S",  "AAPL"                          },
        {"t",  "2024-05-01T12:34:56.123456789Z"},
        {"o",  150.0                           },
        {"h",  151.0                           },
        {"l",  149.5                           },
        {"c",  150.5                           },
        {"v",  1000                            },
        {"n",  12                              },
        {"vw", 150.2                           }
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::UpdatedBar);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->symbol, "AAPL");
    EXPECT_EQ(message->volume, 1000U);
    EXPECT_EQ(message->trade_count, 12U);
    ASSERT_TRUE(message->vwap.has_value());
    EXPECT_DOUBLE_EQ(message->vwap->to_double(), 150.2);
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T12:34:56.123456789Z"));
}

TEST(StreamingTest, RoutesDailyBarMessages) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::DailyBarMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::DailyBarMessage>(msg));
        message = std::get<alpaca::streaming::DailyBarMessage>(msg);
    });

    alpaca::Json payload{
        {"T", "d"                   },
        {"S", "MSFT"                },
        {"t", "2024-05-01T00:00:00Z"},
        {"o", 300.0                 },
        {"h", 305.0                 },
        {"l", 295.0                 },
        {"c", 304.0                 },
        {"v", 5000000               },
        {"n", 42000                 }
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::DailyBar);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->symbol, "MSFT");
    EXPECT_EQ(message->volume, 5000000U);
    EXPECT_EQ(message->trade_count, 42000U);
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T00:00:00Z"));
}

TEST(StreamingTest, RoutesTradeCancelMessages) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::TradeCancelMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::TradeCancelMessage>(msg));
        message = std::get<alpaca::streaming::TradeCancelMessage>(msg);
    });

    alpaca::Json payload{
        {"T", "x"                             },
        {"S", "NFLX"                          },
        {"t", "2024-05-01T14:03:00.000000000Z"},
        {"x", "XNYS"                          },
        {"p", 550.25                          },
        {"s", 25                              },
        {"i", "987654"                        },
        {"a", "cancel"                        },
        {"z", "C"                             }
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::TradeCancel);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->symbol, "NFLX");
    EXPECT_EQ(message->exchange, "XNYS");
    ASSERT_TRUE(message->price.has_value());
    EXPECT_DOUBLE_EQ(message->price->to_double(), 550.25);
    ASSERT_TRUE(message->size.has_value());
    EXPECT_EQ(*message->size, 25U);
    ASSERT_TRUE(message->id.has_value());
    EXPECT_EQ(*message->id, "987654");
    ASSERT_TRUE(message->action.has_value());
    EXPECT_EQ(*message->action, "cancel");
    ASSERT_TRUE(message->tape.has_value());
    EXPECT_EQ(*message->tape, "C");
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T14:03:00.000000000Z"));
}

TEST(StreamingTest, RoutesTradeCorrectionMessages) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::TradeCorrectionMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::TradeCorrectionMessage>(msg));
        message = std::get<alpaca::streaming::TradeCorrectionMessage>(msg);
    });

    alpaca::Json payload{
        {"T",  "c"                             },
        {"S",  "NVDA"                          },
        {"t",  "2024-05-01T15:10:00.000000000Z"},
        {"x",  "XNAS"                          },
        {"oi", "111"                           },
        {"op", 800.5                           },
        {"os", 10                              },
        {"oc", alpaca::Json::array({"@"})      },
        {"ci", "222"                           },
        {"cp", 802.25                          },
        {"cs", 12                              },
        {"cc", alpaca::Json::array({"@", "T"}) },
        {"z",  "C"                             }
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::TradeCorrection);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->symbol, "NVDA");
    EXPECT_EQ(message->exchange, "XNAS");
    ASSERT_TRUE(message->original_id.has_value());
    EXPECT_EQ(*message->original_id, "111");
    ASSERT_TRUE(message->original_price.has_value());
    EXPECT_DOUBLE_EQ(message->original_price->to_double(), 800.5);
    ASSERT_TRUE(message->original_size.has_value());
    EXPECT_EQ(*message->original_size, 10U);
    ASSERT_EQ(message->original_conditions.size(), 1U);
    EXPECT_EQ(message->original_conditions.front(), "@");
    ASSERT_TRUE(message->corrected_id.has_value());
    EXPECT_EQ(*message->corrected_id, "222");
    ASSERT_TRUE(message->corrected_price.has_value());
    EXPECT_DOUBLE_EQ(message->corrected_price->to_double(), 802.25);
    ASSERT_TRUE(message->corrected_size.has_value());
    EXPECT_EQ(*message->corrected_size, 12U);
    ASSERT_EQ(message->corrected_conditions.size(), 2U);
    EXPECT_EQ(message->corrected_conditions[0], "@");
    EXPECT_EQ(message->corrected_conditions[1], "T");
    ASSERT_TRUE(message->tape.has_value());
    EXPECT_EQ(*message->tape, "C");
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T15:10:00.000000000Z"));
}

TEST(StreamingTest, AsyncSubscribeQueuesMessageWhileDisconnected) {
    auto client = make_client();

    alpaca::streaming::MarketSubscription subscription;
    subscription.trades.push_back("AAPL");

    auto future = client.subscribe_async(subscription);
    EXPECT_NO_THROW(future.get());

    EXPECT_EQ(WebSocketClientHarness::pending_message_count(client), 0U);
}

TEST(StreamingTest, AsyncSendRawBuffersMessageUntilConnected) {
    auto client = make_client();

    auto future = client.send_raw_async(alpaca::Json{
        {"action", "noop"}
    });
    EXPECT_NO_THROW(future.get());

    EXPECT_EQ(WebSocketClientHarness::pending_message_count(client), 1U);
}

TEST(StreamingTest, RoutesNewsMessages) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::NewsMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::NewsMessage>(msg));
        message = std::get<alpaca::streaming::NewsMessage>(msg);
    });

    alpaca::Json payload{
        {"T",          "n"                             },
        {"id",         "news-1"                        },
        {"headline",   "Breaking headline"             },
        {"author",     "Reporter"                      },
        {"summary",    "Summary"                       },
        {"content",    "Full content"                  },
        {"url",        "https://example.com/article"   },
        {"source",     "TestWire"                      },
        {"symbols",    alpaca::Json::array({"AAPL"})   },
        {"created_at", "2024-05-01T15:10:00.000000000Z"}
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::News);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->id, "news-1");
    EXPECT_EQ(message->headline, "Breaking headline");
    ASSERT_EQ(message->symbols.size(), 1U);
    EXPECT_EQ(message->symbols.front(), "AAPL");
    ASSERT_TRUE(message->created_at.has_value());
    EXPECT_EQ(*message->created_at, parse_timestamp("2024-05-01T15:10:00.000000000Z"));
}

TEST(StreamingTest, RoutesImbalanceMessages) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::ImbalanceMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::ImbalanceMessage>(msg));
        message = std::get<alpaca::streaming::ImbalanceMessage>(msg);
    });

    alpaca::Json payload{
        {"T",      "i"                             },
        {"S",      "TSLA"                          },
        {"t",      "2024-05-01T15:55:00.000000000Z"},
        {"x",      "XNYS"                          },
        {"im",     4500                            },
        {"paired", 12500                           },
        {"rp",     180.25                          },
        {"np",     180.1                           },
        {"fp",     180.4                           },
        {"cp",     180.3                           },
        {"p",      180.35                          },
        {"side",   "B"                             },
        {"at",     "closing"                       },
        {"z",      "C"                             }
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::Imbalance);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->symbol, "TSLA");
    ASSERT_TRUE(message->exchange.has_value());
    EXPECT_EQ(*message->exchange, "XNYS");
    ASSERT_TRUE(message->imbalance.has_value());
    EXPECT_EQ(*message->imbalance, 4500U);
    ASSERT_TRUE(message->paired.has_value());
    EXPECT_EQ(*message->paired, 12500U);
    ASSERT_TRUE(message->reference_price.has_value());
    EXPECT_DOUBLE_EQ(message->reference_price->to_double(), 180.25);
    ASSERT_TRUE(message->near_price.has_value());
    EXPECT_DOUBLE_EQ(message->near_price->to_double(), 180.1);
    ASSERT_TRUE(message->far_price.has_value());
    EXPECT_DOUBLE_EQ(message->far_price->to_double(), 180.4);
    ASSERT_TRUE(message->current_price.has_value());
    EXPECT_DOUBLE_EQ(message->current_price->to_double(), 180.3);
    ASSERT_TRUE(message->clearing_price.has_value());
    EXPECT_DOUBLE_EQ(message->clearing_price->to_double(), 180.35);
    ASSERT_TRUE(message->imbalance_side.has_value());
    EXPECT_EQ(*message->imbalance_side, "B");
    ASSERT_TRUE(message->auction_type.has_value());
    EXPECT_EQ(*message->auction_type, "closing");
    ASSERT_TRUE(message->tape.has_value());
    EXPECT_EQ(*message->tape, "C");
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T15:55:00.000000000Z"));
    EXPECT_EQ(message->raw_payload, payload);
}

TEST(StreamingTest, RoutesUnderlyingWhenPayloadIncludesUnderlyingSymbol) {
    auto client = make_client();
    std::optional<MessageCategory> category;
    std::optional<alpaca::streaming::UnderlyingMessage> message;

    client.set_message_handler([&category, &message](StreamMessage const& msg, MessageCategory observed_category) {
        category = observed_category;
        ASSERT_TRUE(std::holds_alternative<alpaca::streaming::UnderlyingMessage>(msg));
        message = std::get<alpaca::streaming::UnderlyingMessage>(msg);
    });

    alpaca::Json payload{
        {"T",  "u"                   },
        {"S",  "AAPL240621C00150000" },
        {"uS", "AAPL"                },
        {"t",  "2024-05-01T16:00:00Z"},
        {"p",  150.0                 }
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_TRUE(category.has_value());
    EXPECT_EQ(*category, MessageCategory::Underlying);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->symbol, "AAPL240621C00150000");
    EXPECT_EQ(message->underlying_symbol, "AAPL");
    EXPECT_DOUBLE_EQ(message->price.to_double(), 150.0);
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T16:00:00Z"));
}

TEST(StreamingTest, DetectsSequenceGapsAndRequestsReplay) {
    auto client = make_client();
    client.set_message_handler([](StreamMessage const&, MessageCategory) {
    });

    std::vector<std::tuple<std::string, std::uint64_t, std::uint64_t>> gaps;
    std::vector<std::tuple<std::string, std::uint64_t, std::uint64_t>> replays;

    alpaca::streaming::SequenceGapPolicy policy{};
    policy.stream_identifier = [](alpaca::Json const& payload) {
        if (payload.contains("S")) {
            return payload.at("S").get<std::string>();
        }
        return std::string{};
    };
    policy.sequence_extractor = [](alpaca::Json const& payload) -> std::optional<std::uint64_t> {
        if (!payload.contains("i")) {
            return std::nullopt;
        }
        auto const& field = payload.at("i");
        if (field.is_number_unsigned()) {
            return field.get<std::uint64_t>();
        }
        if (field.is_string()) {
            try {
                return static_cast<std::uint64_t>(std::stoull(field.get<std::string>()));
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    };
    policy.gap_handler = [&gaps](std::string const& stream, std::uint64_t expected, std::uint64_t observed,
                                 alpaca::Json const&) {
        gaps.emplace_back(stream, expected, observed);
    };
    policy.replay_request = [&replays](std::string const& stream, std::uint64_t from_seq, std::uint64_t to_seq,
                                       alpaca::Json const&) {
        replays.emplace_back(stream, from_seq, to_seq);
    };

    client.set_sequence_gap_policy(std::move(policy));

    alpaca::Json base{
        {"T", "t"                   },
        {"S", "AAPL"                },
        {"p", 100.0                 },
        {"s", 10                    },
        {"x", "XNAS"                },
        {"t", "2024-05-01T12:00:00Z"}
    };

    auto first = base;
    first["i"] = "1";
    WebSocketClientHarness::feed(client, first);

    auto second = base;
    second["i"] = "2";
    WebSocketClientHarness::feed(client, second);

    auto third = base;
    third["i"] = "5";
    WebSocketClientHarness::feed(client, third);

    ASSERT_EQ(gaps.size(), 1U);
    EXPECT_EQ(std::get<0>(gaps.front()), "AAPL");
    EXPECT_EQ(std::get<1>(gaps.front()), 3U);
    EXPECT_EQ(std::get<2>(gaps.front()), 5U);

    ASSERT_EQ(replays.size(), 1U);
    EXPECT_EQ(std::get<0>(replays.front()), "AAPL");
    EXPECT_EQ(std::get<1>(replays.front()), 3U);
    EXPECT_EQ(std::get<2>(replays.front()), 4U);
}

TEST(StreamingTest, ReportsLatencyWhenThresholdExceeded) {
    auto client = make_client();
    client.set_message_handler([](StreamMessage const&, MessageCategory) {
    });

    std::vector<std::pair<std::string, std::chrono::nanoseconds>> latency_events;

    alpaca::streaming::LatencyMonitor monitor{};
    monitor.max_latency = std::chrono::milliseconds{10};
    monitor.timestamp_extractor = [](alpaca::Json const& payload) -> std::optional<alpaca::Timestamp> {
        if (!payload.contains("t")) {
            return std::nullopt;
        }
        return parse_timestamp(payload.at("t").get<std::string>());
    };
    monitor.stream_identifier = [](alpaca::Json const& payload) {
        if (payload.contains("S")) {
            return payload.at("S").get<std::string>();
        }
        return std::string{};
    };
    monitor.latency_handler = [&latency_events](std::string const& stream, std::chrono::nanoseconds latency,
                                                alpaca::Json const&) {
        latency_events.emplace_back(stream, latency);
    };

    client.set_latency_monitor(std::move(monitor));

    auto event_time = std::chrono::time_point_cast<alpaca::Timestamp::duration>(std::chrono::system_clock::now() -
                                                                                std::chrono::seconds(2));
    alpaca::Json payload{
        {"T", "t"                                 },
        {"S", "MSFT"                              },
        {"i", "10"                                },
        {"p", 350.0                               },
        {"s", 5                                   },
        {"x", "XNAS"                              },
        {"t", alpaca::format_timestamp(event_time)}
    };

    WebSocketClientHarness::feed(client, payload);

    ASSERT_EQ(latency_events.size(), 1U);
    EXPECT_EQ(latency_events.front().first, "MSFT");
    EXPECT_GT(latency_events.front().second, std::chrono::milliseconds{10});
}

TEST(StreamingTest, IssuesRestBackfillRequestWhenTradeSequenceGapDetected) {
    auto http = std::make_shared<FakeHttpClient>();
    http->push_response(MakeHttpResponse(R"({"trades":{"AAPL":[]}})"));

    alpaca::Configuration config = alpaca::Configuration::Paper("key", "secret");
    alpaca::MarketDataClient market(config, http);

    auto coordinator =
    std::make_shared<alpaca::streaming::BackfillCoordinator>(market, alpaca::streaming::StreamFeed::MarketData);

    WebSocketClient client{"wss://example.com", "key", "secret"};
    client.set_message_handler([](StreamMessage const&, MessageCategory) {
    });
    client.enable_automatic_backfill(coordinator);

    alpaca::Json base{
        {"T", "t"   },
        {"S", "AAPL"},
        {"p", 100.0 },
        {"s", 10    },
        {"x", "XNAS"}
    };

    auto first = base;
    first["i"] = "10";
    first["t"] = "2024-05-01T12:00:00Z";
    WebSocketClientHarness::feed(client, first);

    auto second = base;
    second["i"] = "11";
    second["t"] = "2024-05-01T12:00:01Z";
    WebSocketClientHarness::feed(client, second);

    auto gap = base;
    gap["i"] = "15";
    gap["t"] = "2024-05-01T12:00:05Z";
    WebSocketClientHarness::feed(client, gap);

    ASSERT_EQ(http->requests().size(), 1U);
    auto const& request = http->requests().front().request;
    EXPECT_NE(request.url.find("/v2/stocks/trades"), std::string::npos);
    EXPECT_NE(request.url.find("symbols=AAPL"), std::string::npos);
    EXPECT_NE(request.url.find("start=2024-05-01T12%3A00%3A01Z"), std::string::npos);
    EXPECT_NE(request.url.find("end=2024-05-01T12%3A00%3A05Z"), std::string::npos);
    EXPECT_NE(request.url.find("limit=3"), std::string::npos);
    EXPECT_NE(request.url.find("sort=asc"), std::string::npos);
}

TEST(StreamingTest, CloseEventSchedulesReconnectAndReplaysSubscriptions) {
    auto client = make_client();

    std::vector<Json> sent_messages;
    std::atomic<int> replay_calls{0};
    std::atomic<int> reconnect_calls{0};

    WebSocketClientTestHooks hooks{};
    hooks.on_send_raw = [&sent_messages](Json const& message) {
        sent_messages.push_back(message);
    };
    hooks.on_replay_subscriptions = [&replay_calls]() {
        replay_calls.fetch_add(1, std::memory_order_relaxed);
    };
    hooks.on_schedule_reconnect = [&reconnect_calls]() {
        reconnect_calls.fetch_add(1, std::memory_order_relaxed);
    };
    WebSocketClientHarness::set_test_hooks(client, std::move(hooks));

    MarketSubscription subscription;
    subscription.trades.push_back("AAPL");
    client.subscribe(subscription);
    client.send_raw(alpaca::Json{
        {"action", "noop"}
    });

    ASSERT_EQ(WebSocketClientHarness::pending_message_count(client), 1U);

    WebSocketClientHarness::set_should_reconnect(client, true);
    WebSocketClientHarness::set_connected(client, true);

    auto before_close = reconnect_calls.load(std::memory_order_relaxed);
    WebSocketClientHarness::simulate_close(client);
    WebSocketClientHarness::drain_reconnect_thread(client);

    EXPECT_GT(reconnect_calls.load(std::memory_order_relaxed), before_close);
    EXPECT_GE(WebSocketClientHarness::reconnect_attempt(client), 1U);

    auto baseline = sent_messages.size();
    WebSocketClientHarness::simulate_open(client);

    EXPECT_EQ(replay_calls.load(std::memory_order_relaxed), 1);
    auto pending_after_open = WebSocketClientHarness::pending_message_count(client);
    if (pending_after_open != 0U) {
        auto snapshot = WebSocketClientHarness::pending_messages(client);
        for (auto const& message : snapshot) {
            ADD_FAILURE() << "Pending message after open: " << message.dump();
        }
    }
    EXPECT_EQ(pending_after_open, 0U);

    ASSERT_GE(sent_messages.size(), baseline + 3U);
    auto start = sent_messages.begin() + static_cast<std::ptrdiff_t>(baseline);
    std::vector<Json> new_messages(start, sent_messages.end());

    auto auth_count = std::count_if(new_messages.begin(), new_messages.end(), [](Json const& msg) {
        return msg.contains("action") && msg.at("action").get<std::string>() == "auth";
    });
    EXPECT_GE(auth_count, 1);

    auto subscribe_count = std::count_if(new_messages.begin(), new_messages.end(), [](Json const& msg) {
        if (!msg.contains("action")) {
            return false;
        }
        if (msg.at("action").get<std::string>() != "subscribe") {
            return false;
        }
        if (!msg.contains("trades")) {
            return false;
        }
        auto symbols = msg.at("trades").get<std::vector<std::string>>();
        return std::find(symbols.begin(), symbols.end(), "AAPL") != symbols.end();
    });
    EXPECT_GE(subscribe_count, 1);

    auto noop_count = std::count_if(new_messages.begin(), new_messages.end(), [](Json const& msg) {
        return msg.contains("action") && msg.at("action").get<std::string>() == "noop";
    });
    EXPECT_GE(noop_count, 1);
}

TEST(StreamingTest, ErrorEventSchedulesReconnect) {
    auto client = make_client();

    std::atomic<int> reconnect_calls{0};
    WebSocketClientTestHooks hooks{};
    hooks.on_schedule_reconnect = [&reconnect_calls]() {
        reconnect_calls.fetch_add(1, std::memory_order_relaxed);
    };
    WebSocketClientHarness::set_test_hooks(client, std::move(hooks));

    WebSocketClientHarness::set_should_reconnect(client, true);
    WebSocketClientHarness::set_connected(client, true);

    WebSocketClientHarness::simulate_error(client, "simulated error");
    WebSocketClientHarness::drain_reconnect_thread(client);

    EXPECT_GE(reconnect_calls.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(WebSocketClientHarness::reconnect_attempt(client), 1U);
}

TEST(StreamingTest, HeartbeatTimeoutSchedulesReconnect) {
    auto client = make_client();

    ReconnectPolicy policy;
    policy.initial_delay = std::chrono::milliseconds{0};
    policy.max_delay = std::chrono::milliseconds{0};
    policy.multiplier = 1.0;
    policy.jitter = std::chrono::milliseconds{0};
    client.set_reconnect_policy(policy);

    std::atomic<int> reconnect_calls{0};
    std::atomic<int> timeout_calls{0};

    WebSocketClientTestHooks hooks{};
    hooks.on_schedule_reconnect = [&reconnect_calls]() {
        reconnect_calls.fetch_add(1, std::memory_order_relaxed);
    };
    hooks.on_handle_heartbeat_timeout = [&timeout_calls]() {
        timeout_calls.fetch_add(1, std::memory_order_relaxed);
    };
    WebSocketClientHarness::set_test_hooks(client, std::move(hooks));

    WebSocketClientHarness::set_should_reconnect(client, true);
    WebSocketClientHarness::set_connected(client, true);

    client.set_heartbeat_timeout(std::chrono::milliseconds{1});
    WebSocketClientHarness::set_last_message_time(client, 0);

    WebSocketClientHarness::trigger_heartbeat_timeout(client);
    WebSocketClientHarness::drain_reconnect_thread(client);

    EXPECT_GE(timeout_calls.load(std::memory_order_relaxed), 1);
    EXPECT_GE(reconnect_calls.load(std::memory_order_relaxed), 1);
}

TEST(StreamingTest, RespondsToPingWithPong) {
    auto client = make_client();

    std::vector<Json> sent_messages;
    WebSocketClientTestHooks hooks{};
    hooks.on_send_raw = [&sent_messages](Json const& message) {
        sent_messages.push_back(message);
    };
    WebSocketClientHarness::set_test_hooks(client, std::move(hooks));

    WebSocketClientHarness::set_connected(client, false);
    WebSocketClientHarness::handle_control(client, Json::object(), "ping");

    ASSERT_EQ(WebSocketClientHarness::pending_message_count(client), 1U);
    ASSERT_EQ(sent_messages.size(), 1U);
    auto const& message = sent_messages.front();
    ASSERT_TRUE(message.contains("action"));
    EXPECT_EQ(message.at("action").get<std::string>(), "pong");
}

} // namespace
