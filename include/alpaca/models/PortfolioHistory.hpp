#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"

namespace alpaca {

/// Represents portfolio history statistics returned by the API.
struct PortfolioHistory {
    std::vector<int64_t> timestamp;
    std::vector<double> equity;
    std::vector<double> profit_loss;
    std::vector<double> profit_loss_pct;
    double base_value{0.0};
    std::string timeframe;
};

void from_json(Json const& j, PortfolioHistory& history);

/// Request parameters accepted by the portfolio history endpoint.
struct PortfolioHistoryRequest {
    std::optional<std::string> period;
    std::optional<std::string> timeframe;
    std::optional<std::string> date_start;
    std::optional<std::string> date_end;
    std::optional<bool> extended_hours;

    [[nodiscard]] QueryParams to_query_params() const;
};

} // namespace alpaca
