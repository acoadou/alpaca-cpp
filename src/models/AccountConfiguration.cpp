#include "alpaca/models/AccountConfiguration.hpp"

namespace alpaca {

void from_json(Json const& j, AccountConfiguration& configuration) {
    configuration.dtbp_check = j.value("dtbp_check", std::string{});
    configuration.no_shorting = j.value("no_shorting", false);
    configuration.trade_confirm_email = j.value("trade_confirm_email", std::string{});
    configuration.suspend_trade = j.value("suspend_trade", false);
    configuration.ptp_no_exception_entry = j.value("ptp_no_exception_entry", false);
    if (j.contains("max_options_trading_level") && !j["max_options_trading_level"].is_null()) {
        configuration.max_options_trading_level =
        static_cast<OptionsTradingLevel>(j["max_options_trading_level"].get<int>());
    } else {
        configuration.max_options_trading_level.reset();
    }
}

void to_json(Json& j, AccountConfiguration const& configuration) {
    j = Json{
        {"dtbp_check",             configuration.dtbp_check            },
        {"no_shorting",            configuration.no_shorting           },
        {"trade_confirm_email",    configuration.trade_confirm_email   },
        {"suspend_trade",          configuration.suspend_trade         },
        {"ptp_no_exception_entry", configuration.ptp_no_exception_entry}
    };
    if (configuration.max_options_trading_level.has_value()) {
        j["max_options_trading_level"] = static_cast<int>(*configuration.max_options_trading_level);
    } else {
        j["max_options_trading_level"] = nullptr;
    }
}

void to_json(Json& j, AccountConfigurationUpdateRequest const& request) {
    j = Json{};
    if (request.dtbp_check.has_value()) {
        j["dtbp_check"] = *request.dtbp_check;
    }
    if (request.no_shorting.has_value()) {
        j["no_shorting"] = *request.no_shorting;
    }
    if (request.trade_confirm_email.has_value()) {
        j["trade_confirm_email"] = *request.trade_confirm_email;
    }
    if (request.suspend_trade.has_value()) {
        j["suspend_trade"] = *request.suspend_trade;
    }
    if (request.ptp_no_exception_entry.has_value()) {
        j["ptp_no_exception_entry"] = *request.ptp_no_exception_entry;
    }
    if (request.max_options_trading_level.has_value()) {
        j["max_options_trading_level"] = static_cast<int>(*request.max_options_trading_level);
    }
}

} // namespace alpaca
