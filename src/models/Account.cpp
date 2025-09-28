#include "alpaca/models/Account.hpp"

#include <charconv>
#include <optional>
#include <string>

#include "alpaca/Exceptions.hpp"

namespace {

alpaca::Money parse_money_field(alpaca::Json const& json, char const* key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null() || (it->is_string() && it->get_ref<std::string const&>().empty())) {
        return alpaca::Money{};
    }

    return it->get<alpaca::Money>();
}

std::optional<alpaca::Money> parse_optional_money(alpaca::Json const& json, char const* key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null() || (it->is_string() && it->get_ref<std::string const&>().empty())) {
        return std::nullopt;
    }

    return it->get<alpaca::Money>();
}

int parse_int_field(alpaca::Json const& json, char const* key, int default_value) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return default_value;
    }

    if (it->is_number_integer()) {
        return it->get<int>();
    }

    if (it->is_string()) {
        auto const& text = it->get_ref<std::string const&>();
        if (text.empty()) {
            return default_value;
        }

        int value = 0;
        auto const* const begin = text.data();
        auto const* const end = text.data() + text.size();
        auto const result = std::from_chars(begin, end, value);
        if (result.ec == std::errc{} && result.ptr == end) {
            return value;
        }
    }

    auto const field = std::string{key};
    throw alpaca::InvalidArgumentException(field, std::string{"Expected integer for field '"} + field + "'");
}

std::optional<int> parse_optional_int(alpaca::Json const& json, char const* key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null() || (it->is_string() && it->get_ref<std::string const&>().empty())) {
        return std::nullopt;
    }

    if (it->is_number_integer()) {
        return it->get<int>();
    }

    if (it->is_string()) {
        auto const& text = it->get_ref<std::string const&>();
        int value = 0;
        auto const* const begin = text.data();
        auto const* const end = text.data() + text.size();
        auto const result = std::from_chars(begin, end, value);
        if (result.ec == std::errc{} && result.ptr == end) {
            return value;
        }
    }

    auto const field = std::string{key};
    throw alpaca::InvalidArgumentException(field, std::string{"Expected integer for field '"} + field + "'");
}

alpaca::AccountStatus parse_account_status(alpaca::Json const& json, char const* key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null() || (it->is_string() && it->get_ref<std::string const&>().empty())) {
        return alpaca::AccountStatus::UNKNOWN;
    }

    if (!it->is_string()) {
        auto const field = std::string{key};
        throw alpaca::InvalidArgumentException(field, std::string{"Expected string for field '"} + field + "'");
    }

    auto const text = it->get<std::string>();
    return alpaca::account_status_from_string(text);
}

std::optional<alpaca::AccountCryptoStatus> parse_optional_crypto_status(alpaca::Json const& json, char const* key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null() || (it->is_string() && it->get_ref<std::string const&>().empty())) {
        return std::nullopt;
    }

    if (!it->is_string()) {
        auto const field = std::string{key};
        throw alpaca::InvalidArgumentException(field, std::string{"Expected string for field '"} + field + "'");
    }

    auto const text = it->get<std::string>();
    return alpaca::account_crypto_status_from_string(text);
}

} // namespace

namespace alpaca {

AccountStatus account_status_from_string(std::string_view value) {
    if (value == "UNKNOWN") {
        return AccountStatus::UNKNOWN;
    }
    if (value == "ACTIVE") {
        return AccountStatus::ACTIVE;
    }
    if (value == "INACTIVE") {
        return AccountStatus::INACTIVE;
    }
    if (value == "ACCOUNT_CLOSED") {
        return AccountStatus::ACCOUNT_CLOSED;
    }
    if (value == "ONBOARDING") {
        return AccountStatus::ONBOARDING;
    }
    if (value == "SUBMITTED") {
        return AccountStatus::SUBMITTED;
    }

    throw InvalidArgumentException("value", "Unrecognized account status: " + std::string{value});
}

std::string to_string(AccountStatus status) {
    switch (status) {
    case AccountStatus::UNKNOWN:
        return "UNKNOWN";
    case AccountStatus::ACTIVE:
        return "ACTIVE";
    case AccountStatus::INACTIVE:
        return "INACTIVE";
    case AccountStatus::ACCOUNT_CLOSED:
        return "ACCOUNT_CLOSED";
    case AccountStatus::ONBOARDING:
        return "ONBOARDING";
    case AccountStatus::SUBMITTED:
        return "SUBMITTED";
    }

    throw InvalidArgumentException("status", "Invalid account status value");
}

AccountCryptoStatus account_crypto_status_from_string(std::string_view value) {
    if (value == "UNKNOWN") {
        return AccountCryptoStatus::UNKNOWN;
    }
    if (value == "ACTIVE") {
        return AccountCryptoStatus::ACTIVE;
    }
    if (value == "INACTIVE") {
        return AccountCryptoStatus::INACTIVE;
    }

    throw InvalidArgumentException("value", "Unrecognized crypto account status: " + std::string{value});
}

std::string to_string(AccountCryptoStatus status) {
    switch (status) {
    case AccountCryptoStatus::UNKNOWN:
        return "UNKNOWN";
    case AccountCryptoStatus::ACTIVE:
        return "ACTIVE";
    case AccountCryptoStatus::INACTIVE:
        return "INACTIVE";
    }

    throw InvalidArgumentException("status", "Invalid crypto account status value");
}

void from_json(Json const& j, Account& account) {
    j.at("id").get_to(account.id);
    account.account_number = j.value("account_number", std::string{});
    account.currency = j.value("currency", std::string{});
    account.status = parse_account_status(j, "status");
    account.crypto_status = parse_optional_crypto_status(j, "crypto_status");
    account.account_blocked = j.value("account_blocked", false);
    account.trade_blocked = j.value("trade_blocked", false);
    account.trading_blocked = j.value("trading_blocked", false);
    account.transfers_blocked = j.value("transfers_blocked", false);
    account.trade_suspended_by_user = j.value("trade_suspended_by_user", false);
    account.pattern_day_trader = j.value("pattern_day_trader", false);
    account.shorting_enabled = j.value("shorting_enabled", false);
    account.buying_power = parse_money_field(j, "buying_power");
    account.regt_buying_power = parse_money_field(j, "regt_buying_power");
    account.daytrading_buying_power = parse_money_field(j, "daytrading_buying_power");
    account.non_marginable_buying_power = parse_money_field(j, "non_marginable_buying_power");
    account.equity = parse_money_field(j, "equity");
    account.last_equity = parse_money_field(j, "last_equity");
    account.cash = parse_money_field(j, "cash");
    account.accrued_fees = parse_optional_money(j, "accrued_fees");
    account.cash_long = parse_money_field(j, "cash_long");
    account.cash_short = parse_money_field(j, "cash_short");
    account.cash_withdrawable = parse_money_field(j, "cash_withdrawable");
    account.pending_transfer_out = parse_optional_money(j, "pending_transfer_out");
    account.pending_transfer_in = parse_optional_money(j, "pending_transfer_in");
    account.portfolio_value = parse_money_field(j, "portfolio_value");
    account.long_market_value = parse_money_field(j, "long_market_value");
    account.short_market_value = parse_money_field(j, "short_market_value");
    account.initial_margin = parse_money_field(j, "initial_margin");
    account.maintenance_margin = parse_money_field(j, "maintenance_margin");
    account.last_maintenance_margin = parse_money_field(j, "last_maintenance_margin");
    account.multiplier = parse_int_field(j, "multiplier", 0);
    account.sma = parse_money_field(j, "sma");
    account.options_buying_power = parse_money_field(j, "options_buying_power");
    account.options_approved_level = parse_optional_int(j, "options_approved_level");
    account.options_trading_level = parse_optional_int(j, "options_trading_level");
    account.created_at = j.value("created_at", std::string{});
    account.daytrade_count = parse_optional_int(j, "daytrade_count");
}

} // namespace alpaca
