#pragma once

#include <optional>
#include <string>

#include "alpaca/Json.hpp"

namespace alpaca {

/// Represents an Alpaca trading account.
struct Account {
  std::string id;
  std::string account_number;
  std::string currency;
  std::string status;
  bool account_blocked{false};
  bool trade_blocked{false};
  bool trading_blocked{false};
  bool transfers_blocked{false};
  bool pattern_day_trader{false};
  bool shorting_enabled{false};
  std::string buying_power;
  std::string regt_buying_power;
  std::string daytrading_buying_power;
  std::string equity;
  std::string last_equity;
  std::string cash;
  std::string portfolio_value;
  std::string long_market_value;
  std::string short_market_value;
  std::string initial_margin;
  std::string maintenance_margin;
  std::string last_maintenance_margin;
  std::string multiplier;
  std::string created_at;
  std::optional<std::string> daytrade_count;
};

void from_json(const Json& j, Account& account);

}  // namespace alpaca

