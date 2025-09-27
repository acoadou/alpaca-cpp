#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/Position.hpp"

namespace alpaca {

using OptionOrder = Order;
using NewOptionOrderRequest = NewOrderRequest;
using ReplaceOptionOrderRequest = ReplaceOrderRequest;
using ListOptionOrdersRequest = ListOrdersRequest;
using OptionCancelledOrderId = CancelledOrderId;
using CloseOptionPositionRequest = ClosePositionRequest;

/// Represents an open options position within an account.
struct OptionPosition {
    std::string asset_id;
    std::string symbol;
    std::string exchange;
    std::string asset_class;
    std::string account_id;
    std::string qty;
    std::string qty_available;
    std::string avg_entry_price;
    std::string market_value;
    std::string cost_basis;
    std::string unrealized_pl;
    std::string unrealized_plpc;
    std::string unrealized_intraday_pl;
    std::string unrealized_intraday_plpc;
    std::string current_price;
    std::string lastday_price;
    std::string change_today;
    std::string side;
    std::optional<std::string> contract_multiplier{};
    std::optional<std::string> expiry{};
    std::optional<std::string> strike_price{};
    std::optional<std::string> style{};
    std::optional<std::string> type{};
    std::optional<std::string> underlying_symbol{};
};

void from_json(Json const& j, OptionPosition& position);

/// Represents a discoverable option contract returned by the catalog endpoints.
struct OptionContract {
    std::string id;
    std::string symbol;
    std::string status;
    bool tradable{false};
    std::string underlying_symbol;
    std::string expiration_date;
    std::string strike_price;
    std::string type;
    std::string style;
    std::optional<std::string> root_symbol{};
    std::optional<std::string> exchange{};
    std::optional<std::string> exercise_style{};
    std::optional<std::string> multiplier{};
    std::optional<std::uint64_t> open_interest{};
    std::optional<std::string> open_interest_date{};
    std::optional<double> close_price{};
    std::optional<std::string> contract_size{};
    std::optional<std::string> underlying_asset_id{};
};

/// Response payload for option contract listings.
struct OptionContractsResponse {
    std::vector<OptionContract> contracts;
    std::optional<std::string> next_page_token{};
};

/// Request parameters accepted by the option contract discovery endpoint.
struct ListOptionContractsRequest {
    std::vector<std::string> underlying_symbols{};
    std::optional<std::string> status{};
    std::optional<std::string> expiry{};
    std::optional<std::string> type{};
    std::optional<std::string> style{};
    std::optional<std::string> strike{};
    std::optional<std::string> strike_gte{};
    std::optional<std::string> strike_lte{};
    std::optional<int> limit{};
    std::optional<SortDirection> direction{};
    std::optional<std::string> page_token{};

    [[nodiscard]] QueryParams to_query_params() const;
};

/// Collection of greeks reported by the option analytics endpoint.
struct OptionGreeks {
    std::optional<double> delta{};
    std::optional<double> gamma{};
    std::optional<double> theta{};
    std::optional<double> vega{};
    std::optional<double> rho{};
};

/// Risk parameters provided by the option analytics endpoint.
struct OptionRiskParameters {
    std::optional<double> implied_volatility{};
    std::optional<double> theoretical_price{};
    std::optional<double> underlying_price{};
    std::optional<double> breakeven_price{};
};

/// A single leg that composes a multi-leg options strategy.
struct OptionStrategyLeg {
    std::string symbol;
    OrderSide side{OrderSide::BUY};
    int ratio{1};
};

/// Analytic payload describing greeks and optional strategy legs for a contract.
struct OptionAnalytics {
    std::string symbol;
    std::optional<OptionGreeks> greeks{};
    std::optional<OptionRiskParameters> risk_parameters{};
    std::optional<double> implied_volatility{};
    std::vector<OptionStrategyLeg> legs{};
};

/// Response envelope returned by the options analytics endpoint.
struct OptionAnalyticsResponse {
    std::vector<OptionAnalytics> analytics;
    std::optional<std::string> next_page_token{};
};

/// Request parameters accepted by the options analytics endpoint.
struct ListOptionAnalyticsRequest {
    std::vector<std::string> symbols{};
    std::optional<std::string> underlying_symbol{};
    std::optional<bool> include_greeks{};
    std::optional<bool> include_risk_parameters{};
    std::optional<int> limit{};
    std::optional<std::string> page_token{};

    [[nodiscard]] QueryParams to_query_params() const;
};

void from_json(Json const& j, OptionContract& contract);
void from_json(Json const& j, OptionContractsResponse& response);
void from_json(Json const& j, OptionGreeks& greeks);
void from_json(Json const& j, OptionRiskParameters& risk);
void from_json(Json const& j, OptionStrategyLeg& leg);
void from_json(Json const& j, OptionAnalytics& analytics);
void from_json(Json const& j, OptionAnalyticsResponse& response);

} // namespace alpaca
