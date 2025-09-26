#include "alpaca/models/PortfolioHistory.hpp"

namespace alpaca {

void from_json(const Json& j, PortfolioHistory& history) {
  history.timestamp = j.value("timestamp", std::vector<int64_t>{});
  history.equity = j.value("equity", std::vector<double>{});
  history.profit_loss = j.value("profit_loss", std::vector<double>{});
  history.profit_loss_pct = j.value("profit_loss_pct", std::vector<double>{});
  history.base_value = j.value("base_value", 0.0);
  history.timeframe = j.value("timeframe", std::string{});
}

QueryParams PortfolioHistoryRequest::to_query_params() const {
  QueryParams params;
  if (period.has_value()) {
    params.emplace_back("period", *period);
  }
  if (timeframe.has_value()) {
    params.emplace_back("timeframe", *timeframe);
  }
  if (date_start.has_value()) {
    params.emplace_back("date_start", *date_start);
  }
  if (date_end.has_value()) {
    params.emplace_back("date_end", *date_end);
  }
  if (extended_hours.has_value()) {
    params.emplace_back("extended_hours", *extended_hours ? "true" : "false");
  }
  return params;
}

}  // namespace alpaca
