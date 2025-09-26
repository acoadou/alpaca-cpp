#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "alpaca/Json.hpp"

namespace alpaca {

using Timestamp = std::chrono::sys_time<std::chrono::nanoseconds>;

/// Enumeration of order sides supported by Alpaca.
enum class OrderSide { BUY, SELL };

/// Enumeration of order types supported by Alpaca.
enum class OrderType { MARKET, LIMIT, STOP, STOP_LIMIT, TRAILING_STOP };

/// Order grouping semantics supported by Alpaca advanced orders.
enum class OrderClass { SIMPLE, BRACKET, ONE_CANCELS_OTHER, ONE_TRIGGERS_OTHER };

/// Duration for which an order remains in force.
enum class TimeInForce { DAY, GTC, OPG, IOC, FOK, GTD };  // NOLINT

/// Status of a traded asset.
enum class AssetStatus { ACTIVE, INACTIVE };  // NOLINT

/// Asset class enumeration.
enum class AssetClass { US_EQUITY, CRYPTO, FOREX, FUTURES };

/// Sort direction used by list endpoints.
enum class SortDirection { ASC, DESC };

/// Helper structure for pagination cursors returned by Alpaca list endpoints.
struct PageToken {
  std::string next;
  std::string prev;
};

std::string to_string(OrderSide side);
std::string to_string(OrderType type);
std::string to_string(TimeInForce tif);
std::string to_string(OrderClass order_class);
std::string to_string(AssetStatus status);
std::string to_string(AssetClass asset_class);
std::string to_string(SortDirection direction);

OrderSide order_side_from_string(const std::string& value);
OrderType order_type_from_string(const std::string& value);
TimeInForce time_in_force_from_string(const std::string& value);
OrderClass order_class_from_string(const std::string& value);
AssetStatus asset_status_from_string(const std::string& value);
AssetClass asset_class_from_string(const std::string& value);
SortDirection sort_direction_from_string(const std::string& value);

void to_json(Json& j, const PageToken& token);
void from_json(const Json& j, PageToken& token);

Timestamp parse_timestamp(std::string_view value);
std::optional<Timestamp> parse_timestamp_field(const Json& j, const char* key);
std::string format_timestamp(Timestamp timestamp);
std::string format_calendar_date(std::chrono::sys_days day);
std::string join_csv(const std::vector<std::string>& values);

/// Parameters describing a take-profit leg of an advanced order.
struct TakeProfitParams {
  std::string limit_price;
};

/// Parameters describing a stop-loss leg of an advanced order.
struct StopLossParams {
  std::optional<std::string> stop_price;
  std::optional<std::string> limit_price;
};

void to_json(Json& j, const TakeProfitParams& params);
void to_json(Json& j, const StopLossParams& params);

/// Request payload sent when cancelling all orders.
struct CancelledOrderId {
  std::string id;
  std::string status;
};

void from_json(const Json& j, CancelledOrderId& id);

}  // namespace alpaca

