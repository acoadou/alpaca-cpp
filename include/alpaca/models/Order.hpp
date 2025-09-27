#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Common fields shared by all new order request payloads.
struct NewOrderBase {
    std::string symbol;
    OrderSide side{OrderSide::BUY};
    OrderType type{OrderType::MARKET};
    TimeInForce time_in_force{TimeInForce::DAY};
    std::optional<std::string> quantity;
    std::optional<std::string> notional;
    std::optional<std::string> limit_price;
    std::optional<std::string> stop_price;
    std::optional<std::string> client_order_id;
    std::optional<OrderClass> order_class;
    std::optional<TakeProfitParams> take_profit;
    std::optional<StopLossParams> stop_loss;
};

/// Request used to submit a new equity order.
struct NewOrderRequest : NewOrderBase {
    bool extended_hours{false};
};

/// Request used to submit a new multi-asset order with venue routing controls.
struct NewMultiAssetOrderRequest : NewOrderBase {
    std::optional<std::string> base_symbol{};
    std::optional<std::string> quote_symbol{};
    std::optional<std::string> notional_currency{};
    std::optional<std::string> venue{};
    std::optional<std::string> routing_strategy{};
    std::optional<bool> post_only{};
    std::optional<bool> reduce_only{};
};

/// Request used to submit a new crypto spot order.
struct NewCryptoOrderRequest : NewMultiAssetOrderRequest {};

/// Request used to submit a new OTC order.
struct NewOtcOrderRequest : NewMultiAssetOrderRequest {
    std::optional<std::string> counterparty{};
    std::optional<std::string> quote_id{};
    std::optional<std::string> settlement_date{};
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
    std::optional<std::string> base_symbol{};
    std::optional<std::string> quote_symbol{};
    std::optional<std::string> notional_currency{};
    std::optional<std::string> venue{};
    std::optional<std::string> routing_strategy{};
    std::optional<bool> post_only{};
    std::optional<bool> reduce_only{};
    std::optional<std::string> counterparty{};
    std::optional<std::string> quote_id{};
    std::optional<std::string> settlement_date{};
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

/// Request payload used to replace an existing multi-asset order.
struct ReplaceMultiAssetOrderRequest : ReplaceOrderRequest {
    std::optional<std::string> quote_symbol{};
    std::optional<std::string> venue{};
    std::optional<std::string> routing_strategy{};
    std::optional<bool> post_only{};
    std::optional<bool> reduce_only{};
};

/// Request payload used to replace a crypto order.
struct ReplaceCryptoOrderRequest : ReplaceMultiAssetOrderRequest {};

/// Request payload used to replace an OTC order.
struct ReplaceOtcOrderRequest : ReplaceMultiAssetOrderRequest {
    std::optional<std::string> counterparty{};
    std::optional<std::string> settlement_date{};
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
    std::optional<AssetClass> asset_class{};
    std::optional<std::string> venue{};

    [[nodiscard]] QueryParams to_query_params() const;
};

using CryptoOrder = Order;
using CryptoCancelledOrderId = CancelledOrderId;
using ListCryptoOrdersRequest = ListOrdersRequest;

using OtcOrder = Order;
using OtcCancelledOrderId = CancelledOrderId;
using ListOtcOrdersRequest = ListOrdersRequest;

void to_json(Json& j, NewOrderRequest const& request);
void to_json(Json& j, NewCryptoOrderRequest const& request);
void to_json(Json& j, NewOtcOrderRequest const& request);
void from_json(Json const& j, Order& order);
void to_json(Json& j, ReplaceOrderRequest const& request);
void to_json(Json& j, ReplaceCryptoOrderRequest const& request);
void to_json(Json& j, ReplaceOtcOrderRequest const& request);

} // namespace alpaca
