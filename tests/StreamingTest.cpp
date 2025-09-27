#include "alpaca/Streaming.hpp"

#include <gtest/gtest.h>

#include <optional>

#include "alpaca/Json.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca::streaming {

class WebSocketClientHarness {
  public:
    static void feed(WebSocketClient& client, Json const& payload) {
        client.handle_payload(payload);
    }
};

} // namespace alpaca::streaming

namespace {

using alpaca::parse_timestamp;
using alpaca::streaming::MessageCategory;
using alpaca::streaming::StreamMessage;
using alpaca::streaming::WebSocketClient;
using alpaca::streaming::WebSocketClientHarness;

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
    EXPECT_DOUBLE_EQ(*message->vwap, 150.2);
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
    EXPECT_DOUBLE_EQ(*message->price, 550.25);
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
    EXPECT_DOUBLE_EQ(*message->original_price, 800.5);
    ASSERT_TRUE(message->original_size.has_value());
    EXPECT_EQ(*message->original_size, 10U);
    ASSERT_EQ(message->original_conditions.size(), 1U);
    EXPECT_EQ(message->original_conditions.front(), "@");
    ASSERT_TRUE(message->corrected_id.has_value());
    EXPECT_EQ(*message->corrected_id, "222");
    ASSERT_TRUE(message->corrected_price.has_value());
    EXPECT_DOUBLE_EQ(*message->corrected_price, 802.25);
    ASSERT_TRUE(message->corrected_size.has_value());
    EXPECT_EQ(*message->corrected_size, 12U);
    ASSERT_EQ(message->corrected_conditions.size(), 2U);
    EXPECT_EQ(message->corrected_conditions[0], "@");
    EXPECT_EQ(message->corrected_conditions[1], "T");
    ASSERT_TRUE(message->tape.has_value());
    EXPECT_EQ(*message->tape, "C");
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T15:10:00.000000000Z"));
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
    EXPECT_DOUBLE_EQ(*message->reference_price, 180.25);
    ASSERT_TRUE(message->near_price.has_value());
    EXPECT_DOUBLE_EQ(*message->near_price, 180.1);
    ASSERT_TRUE(message->far_price.has_value());
    EXPECT_DOUBLE_EQ(*message->far_price, 180.4);
    ASSERT_TRUE(message->current_price.has_value());
    EXPECT_DOUBLE_EQ(*message->current_price, 180.3);
    ASSERT_TRUE(message->clearing_price.has_value());
    EXPECT_DOUBLE_EQ(*message->clearing_price, 180.35);
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
    EXPECT_DOUBLE_EQ(message->price, 150.0);
    EXPECT_EQ(message->timestamp, parse_timestamp("2024-05-01T16:00:00Z"));
}

} // namespace
