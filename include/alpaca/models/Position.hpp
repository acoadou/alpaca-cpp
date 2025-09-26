#pragma once

#include <optional>
#include <string>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

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

void from_json(const Json& j, Position& position);

/// Request parameters accepted by the close position endpoint.
struct ClosePositionRequest {
  std::optional<std::string> quantity{};
  std::optional<double> percentage{};
  std::optional<TimeInForce> time_in_force{};
  std::optional<double> limit_price{};
  std::optional<double> stop_price{};

  [[nodiscard]] QueryParams to_query_params() const;
};

}  // namespace alpaca

