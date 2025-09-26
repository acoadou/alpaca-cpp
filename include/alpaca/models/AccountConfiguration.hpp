#pragma once

#include <optional>
#include <string>

#include "alpaca/Json.hpp"

namespace alpaca {

/// Represents account configuration settings returned by the Alpaca API.
struct AccountConfiguration {
  std::string dtbp_check;
  bool no_shorting{false};
  std::string trade_confirm_email;
  bool suspend_trade{false};
};

void from_json(const Json& j, AccountConfiguration& configuration);
void to_json(Json& j, const AccountConfiguration& configuration);

/// Partial update payload for the account configuration endpoint.
struct AccountConfigurationUpdateRequest {
  std::optional<std::string> dtbp_check;
  std::optional<bool> no_shorting;
  std::optional<std::string> trade_confirm_email;
  std::optional<bool> suspend_trade;
};

void to_json(Json& j, const AccountConfigurationUpdateRequest& request);

}  // namespace alpaca
