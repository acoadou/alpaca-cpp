#include "alpaca/models/Position.hpp"

namespace alpaca {

void from_json(const Json& j, Position& position) {
  j.at("asset_id").get_to(position.asset_id);
  position.symbol = j.value("symbol", std::string{});
  position.exchange = j.value("exchange", std::string{});
  position.asset_class = j.value("asset_class", std::string{});
  position.qty = j.value("qty", std::string{});
  position.qty_available = j.value("qty_available", std::string{});
  position.avg_entry_price = j.value("avg_entry_price", std::string{});
  position.market_value = j.value("market_value", std::string{});
  position.cost_basis = j.value("cost_basis", std::string{});
  position.unrealized_pl = j.value("unrealized_pl", std::string{});
  position.unrealized_plpc = j.value("unrealized_plpc", std::string{});
  position.unrealized_intraday_pl = j.value("unrealized_intraday_pl", std::string{});
  position.unrealized_intraday_plpc = j.value("unrealized_intraday_plpc", std::string{});
  position.current_price = j.value("current_price", std::string{});
  position.lastday_price = j.value("lastday_price", std::string{});
  position.change_today = j.value("change_today", std::string{});
}

QueryParams ClosePositionRequest::to_query_params() const {
  QueryParams params;
  if (quantity.has_value()) {
    params.emplace_back("qty", *quantity);
  }
  if (percentage.has_value()) {
    params.emplace_back("percentage", std::to_string(*percentage));
  }
  if (time_in_force.has_value()) {
    params.emplace_back("time_in_force", to_string(*time_in_force));
  }
  if (limit_price.has_value()) {
    params.emplace_back("limit_price", std::to_string(*limit_price));
  }
  if (stop_price.has_value()) {
    params.emplace_back("stop_price", std::to_string(*stop_price));
  }
  return params;
}

}  // namespace alpaca
