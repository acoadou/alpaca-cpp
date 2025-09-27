#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Request used to submit a new order.
struct NewOrderRequest {
    std::string symbol;
    OrderSide side{OrderSide::BUY};
    OrderType type{OrderType::MARKET};
    TimeInForce time_in_force{TimeInForce::DAY};
    std::optional<std::string> quantity;
    std::optional<std::string> notional;
    std::optional<std::string> limit_price;
    std::optional<std::string> stop_price;
    std::optional<std::string> client_order_id;
    bool extended_hours{false};
    std::optional<OrderClass> order_class;
    std::optional<TakeProfitParams> take_profit;
    std::optional<StopLossParams> stop_loss;
};

/// Represents an order returned by the Alpaca API.
struct Order {
    std::string id;
    std::string asset_id;
    std::string client_order_id;
    std::string account_id;
    Timestamp created_at{};
    std::optional<Timestamp> updated_at{};
    std::optional<Timestamp> submitted_at{};
    std::optional<Timestamp> filled_at{};
    std::optional<Timestamp> expired_at{};
    std::optional<Timestamp> canceled_at{};
    std::optional<Timestamp> failed_at{};
    std::optional<Timestamp> replaced_at{};
    std::string replaced_by;
    std::string replaces;
    std::string symbol;
    std::string asset_class;
    OrderSide side{OrderSide::BUY};
    OrderType type{OrderType::MARKET};
    TimeInForce time_in_force{TimeInForce::DAY};
    std::optional<OrderClass> order_class;
    std::string status;
    std::optional<std::string> qty;
    std::optional<std::string> notional;
    std::optional<std::string> filled_qty;
    std::optional<std::string> filled_avg_price;
    std::optional<std::string> limit_price;
    std::optional<std::string> stop_price;
    bool extended_hours{false};
    std::vector<Order> legs;
};

/// Filters available for the list orders endpoint.
enum class OrderStatusFilter {
    OPEN,
    CLOSED,
    ALL
};

std::string to_string(OrderStatusFilter status);

/// Request payload used to replace an existing order.
struct ReplaceOrderRequest {
    std::optional<std::string> quantity;
    std::optional<std::string> time_in_force;
    std::optional<std::string> limit_price;
    std::optional<std::string> stop_price;
    std::optional<bool> extended_hours;
    std::optional<std::string> client_order_id;
};

/// Request parameters accepted by the list orders endpoint.
struct ListOrdersRequest {
    std::optional<OrderStatusFilter> status{};
    std::optional<int> limit{};
    std::optional<Timestamp> after{};
    std::optional<Timestamp> until{};
    std::optional<SortDirection> direction{};
    std::optional<bool> nested{};
    std::vector<std::string> symbols{};

    [[nodiscard]] QueryParams to_query_params() const;
};

void to_json(Json& j, NewOrderRequest const& request);
void from_json(Json const& j, Order& order);
void to_json(Json& j, ReplaceOrderRequest const& request);

} // namespace alpaca
