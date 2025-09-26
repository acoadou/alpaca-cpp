#include "alpaca/models/AccountConfiguration.hpp"

namespace alpaca {

void from_json(const Json& j, AccountConfiguration& configuration) {
  configuration.dtbp_check = j.value("dtbp_check", std::string{});
  configuration.no_shorting = j.value("no_shorting", false);
  configuration.trade_confirm_email = j.value("trade_confirm_email", std::string{});
  configuration.suspend_trade = j.value("suspend_trade", false);
}

void to_json(Json& j, const AccountConfiguration& configuration) {
  j = Json{{"dtbp_check", configuration.dtbp_check},
           {"no_shorting", configuration.no_shorting},
           {"trade_confirm_email", configuration.trade_confirm_email},
           {"suspend_trade", configuration.suspend_trade}};
}

void to_json(Json& j, const AccountConfigurationUpdateRequest& request) {
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
}

}  // namespace alpaca
