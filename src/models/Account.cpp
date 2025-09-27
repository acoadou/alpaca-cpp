#include "alpaca/models/Account.hpp"

namespace alpaca {

void from_json(Json const& j, Account& account) {
    j.at("id").get_to(account.id);
    account.account_number = j.value("account_number", std::string{});
    account.currency = j.value("currency", std::string{});
    account.status = j.value("status", std::string{});
    account.account_blocked = j.value("account_blocked", false);
    account.trade_blocked = j.value("trade_blocked", false);
    account.trading_blocked = j.value("trading_blocked", false);
    account.transfers_blocked = j.value("transfers_blocked", false);
    account.pattern_day_trader = j.value("pattern_day_trader", false);
    account.shorting_enabled = j.value("shorting_enabled", false);
    account.buying_power = j.value("buying_power", std::string{});
    account.regt_buying_power = j.value("regt_buying_power", std::string{});
    account.daytrading_buying_power = j.value("daytrading_buying_power", std::string{});
    account.equity = j.value("equity", std::string{});
    account.last_equity = j.value("last_equity", std::string{});
    account.cash = j.value("cash", std::string{});
    account.portfolio_value = j.value("portfolio_value", std::string{});
    account.long_market_value = j.value("long_market_value", std::string{});
    account.short_market_value = j.value("short_market_value", std::string{});
    account.initial_margin = j.value("initial_margin", std::string{});
    account.maintenance_margin = j.value("maintenance_margin", std::string{});
    account.last_maintenance_margin = j.value("last_maintenance_margin", std::string{});
    account.multiplier = j.value("multiplier", std::string{});
    account.created_at = j.value("created_at", std::string{});
    if (j.contains("daytrade_count") && !j.at("daytrade_count").is_null()) {
        account.daytrade_count = j.at("daytrade_count").get<std::string>();
    } else {
        account.daytrade_count.reset();
    }
}

} // namespace alpaca
