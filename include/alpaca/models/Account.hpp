#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "alpaca/Json.hpp"
#include "alpaca/Money.hpp"

namespace alpaca {

/// Status of a primary Alpaca account.
enum class AccountStatus {
    UNKNOWN,
    ACTIVE,
    INACTIVE,
    ACCOUNT_CLOSED,
    ONBOARDING,
    SUBMITTED
};

/// Status of the crypto sub-account, when applicable.
enum class AccountCryptoStatus {
    UNKNOWN,
    ACTIVE,
    INACTIVE
};

AccountStatus account_status_from_string(std::string_view value);
std::string to_string(AccountStatus status);

AccountCryptoStatus account_crypto_status_from_string(std::string_view value);
std::string to_string(AccountCryptoStatus status);

/// Represents an Alpaca trading account.
struct Account {
    std::string id;
    std::string account_number;
    std::string currency;
    AccountStatus status{AccountStatus::UNKNOWN};
    std::optional<AccountCryptoStatus> crypto_status;
    bool account_blocked{false};
    bool trade_blocked{false};
    bool trading_blocked{false};
    bool transfers_blocked{false};
    bool trade_suspended_by_user{false};
    bool pattern_day_trader{false};
    bool shorting_enabled{false};
    Money buying_power{};
    Money regt_buying_power{};
    Money daytrading_buying_power{};
    Money non_marginable_buying_power{};
    Money equity{};
    Money last_equity{};
    Money cash{};
    std::optional<Money> accrued_fees;
    Money cash_long{};
    Money cash_short{};
    Money cash_withdrawable{};
    std::optional<Money> pending_transfer_out;
    std::optional<Money> pending_transfer_in;
    Money portfolio_value{};
    Money long_market_value{};
    Money short_market_value{};
    Money initial_margin{};
    Money maintenance_margin{};
    Money last_maintenance_margin{};
    int multiplier{0};
    Money sma{};
    Money options_buying_power{};
    std::optional<int> options_approved_level;
    std::optional<int> options_trading_level;
    std::string created_at;
    std::optional<int> daytrade_count;
};

void from_json(Json const& j, Account& account);

} // namespace alpaca
