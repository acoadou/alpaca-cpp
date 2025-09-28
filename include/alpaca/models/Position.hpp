#pragma once

#include <optional>
#include <string>
#include <variant>

#include "alpaca/Json.hpp"
#include "alpaca/Money.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"
#include "alpaca/models/Order.hpp"

namespace alpaca {

/// Represents an open position within an account.
struct Position {
    std::string asset_id;
    std::string symbol;
    std::string exchange;
    std::string asset_class;
    std::string qty;
    std::string qty_available;
    std::string avg_entry_price;
    std::string market_value;
    std::string cost_basis;
    std::string unrealized_pl;
    std::string unrealized_plpc;
    std::string unrealized_intraday_pl;
    std::string unrealized_intraday_plpc;
    std::string current_price;
    std::string lastday_price;
    std::string change_today;
};

/// Request parameters accepted by the close position endpoint.
struct ClosePositionRequest {
    std::optional<std::string> quantity{};
    std::optional<double> percentage{};
    std::optional<TimeInForce> time_in_force{};
    std::optional<Money> limit_price{};
    std::optional<Money> stop_price{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Request parameters accepted by the close all positions endpoint.
struct CloseAllPositionsRequest {
    std::optional<bool> cancel_orders{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Additional information returned when a close position request fails.
struct FailedClosePositionDetails {
    std::optional<int> code{};
    std::optional<std::string> message{};
    std::optional<double> available{};
    std::optional<double> existing_qty{};
    std::optional<double> held_for_orders{};
    std::optional<std::string> symbol{};
};

using ClosePositionBody = std::variant<std::monostate, Order, FailedClosePositionDetails>;

/// Response returned by close position requests.
struct ClosePositionResponse {
    std::optional<std::string> order_id{};
    std::optional<int> status{};
    std::optional<std::string> symbol{};
    ClosePositionBody body{};
};

void from_json(Json const& j, Position& position);
void from_json(Json const& j, FailedClosePositionDetails& details);
void from_json(Json const& j, ClosePositionResponse& response);

} // namespace alpaca
