#pragma once

#include <optional>
#include <string>

#include "alpaca/Json.hpp"

namespace alpaca {

/// Represents account configuration settings returned by the Alpaca API.
enum class OptionsTradingLevel {
    DISABLED = 0,
    COVERED = 1,
    LONG = 2,
    SPREADS = 3,
};

struct AccountConfiguration {
    std::string dtbp_check;
    bool no_shorting{false};
    std::string trade_confirm_email;
    bool suspend_trade{false};
    bool ptp_no_exception_entry{false};
    std::optional<OptionsTradingLevel> max_options_trading_level;
};

void from_json(Json const& j, AccountConfiguration& configuration);
void to_json(Json& j, AccountConfiguration const& configuration);

/// Partial update payload for the account configuration endpoint.
struct AccountConfigurationUpdateRequest {
    std::optional<std::string> dtbp_check;
    std::optional<bool> no_shorting;
    std::optional<std::string> trade_confirm_email;
    std::optional<bool> suspend_trade;
    std::optional<bool> ptp_no_exception_entry;
    std::optional<OptionsTradingLevel> max_options_trading_level;
};

void to_json(Json& j, AccountConfigurationUpdateRequest const& request);

} // namespace alpaca
