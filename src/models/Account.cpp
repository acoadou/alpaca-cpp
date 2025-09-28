#include "alpaca/models/Account.hpp"

#include <cstddef>
#include <exception>
#include <optional>
#include <string>

namespace {

std::optional<double> parse_optional_double(alpaca::Json const& json, char const *key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return std::nullopt;
    }

    if (it->is_number()) {
        return it->get<double>();
    }

    if (it->is_string()) {
        auto const& text = it->get_ref<std::string const&>();
        if (text.empty()) {
            return std::nullopt;
        }

        try {
            std::size_t processed = 0;
            double value = std::stod(text, &processed);
            if (processed == text.size()) {
                return value;
            }
        } catch (std::exception const&) {
        }
    }

    return std::nullopt;
}

std::optional<int> parse_optional_int(alpaca::Json const& json, char const *key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return std::nullopt;
    }

    if (it->is_number_integer()) {
        return it->get<int>();
    }

    if (it->is_string()) {
        auto const& text = it->get_ref<std::string const&>();
        if (text.empty()) {
            return std::nullopt;
        }

        try {
            std::size_t processed = 0;
            int value = std::stoi(text, &processed);
            if (processed == text.size()) {
                return value;
            }
        } catch (std::exception const&) {
        }
    }

    return std::nullopt;
}

std::optional<std::string> parse_optional_string(alpaca::Json const& json, char const *key) {
    auto it = json.find(key);
    if (it == json.end() || it->is_null()) {
        return std::nullopt;
    }

    if (it->is_string()) {
        return it->get<std::string>();
    }

    return std::nullopt;
}

} // namespace

namespace alpaca {

void from_json(Json const& j, Account& account) {
    j.at("id").get_to(account.id);
    account.account_number = j.value("account_number", std::string{});
    account.currency = j.value("currency", std::string{});
    account.status = j.value("status", std::string{});
    account.crypto_status = parse_optional_string(j, "crypto_status");
    account.account_blocked = j.value("account_blocked", false);
    account.trade_blocked = j.value("trade_blocked", false);
    account.trading_blocked = j.value("trading_blocked", false);
    account.transfers_blocked = j.value("transfers_blocked", false);
    account.trade_suspended_by_user = j.value("trade_suspended_by_user", false);
    account.pattern_day_trader = j.value("pattern_day_trader", false);
    account.shorting_enabled = j.value("shorting_enabled", false);
    account.buying_power = j.value("buying_power", std::string{});
    account.regt_buying_power = j.value("regt_buying_power", std::string{});
    account.daytrading_buying_power = j.value("daytrading_buying_power", std::string{});
    account.non_marginable_buying_power = j.value("non_marginable_buying_power", std::string{});
    account.equity = j.value("equity", std::string{});
    account.last_equity = j.value("last_equity", std::string{});
    account.cash = j.value("cash", std::string{});
    account.accrued_fees = parse_optional_double(j, "accrued_fees");
    account.cash_long = j.value("cash_long", std::string{});
    account.cash_short = j.value("cash_short", std::string{});
    account.cash_withdrawable = j.value("cash_withdrawable", std::string{});
    account.pending_transfer_out = parse_optional_double(j, "pending_transfer_out");
    account.pending_transfer_in = parse_optional_double(j, "pending_transfer_in");
    account.portfolio_value = j.value("portfolio_value", std::string{});
    account.long_market_value = j.value("long_market_value", std::string{});
    account.short_market_value = j.value("short_market_value", std::string{});
    account.initial_margin = j.value("initial_margin", std::string{});
    account.maintenance_margin = j.value("maintenance_margin", std::string{});
    account.last_maintenance_margin = j.value("last_maintenance_margin", std::string{});
    account.multiplier = j.value("multiplier", std::string{});
    account.sma = j.value("sma", std::string{});
    account.options_buying_power = j.value("options_buying_power", std::string{});
    account.options_approved_level = parse_optional_int(j, "options_approved_level");
    account.options_trading_level = parse_optional_int(j, "options_trading_level");
    account.created_at = j.value("created_at", std::string{});
    if (j.contains("daytrade_count") && !j.at("daytrade_count").is_null()) {
        account.daytrade_count = j.at("daytrade_count").get<std::string>();
    } else {
        account.daytrade_count.reset();
    }
}

} // namespace alpaca
