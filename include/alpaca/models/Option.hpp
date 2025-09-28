#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/Money.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/Position.hpp"

namespace alpaca {

/// Enumerates option contract types supported by Alpaca.
enum class OptionType {
    CALL,
    PUT
};

/// Enumerates option exercise styles supported by Alpaca.
enum class OptionStyle {
    AMERICAN,
    EUROPEAN
};

/// Enumerates lifecycle statuses reported for option contracts.
enum class OptionStatus {
    ACTIVE,
    HALTED,
    INACTIVE
};

/// Enumerates exchanges on which option contracts are listed.
enum class OptionExchange {
    AMEX,
    ARCA,
    BATS,
    BOX,
    BZX,
    C2,
    CBOE,
    EDGX,
    GEMINI,
    ISE,
    ISE_MERCURY,
    MIAX,
    MIAX_EMERALD,
    MIAX_PEARL,
    NASDAQ,
    NASDAQ_BX,
    NASDAQ_OMX,
    NASDAQ_PHLX,
    NYSE,
    NYSE_ARCA,
    OPRA
};

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
    std::optional<OptionExchange> exchange{};
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
    std::optional<OptionStyle> style{};
    std::optional<OptionType> type{};
    std::optional<std::string> underlying_symbol{};
};

void from_json(Json const& j, OptionPosition& position);

/// Represents a discoverable option contract returned by the catalog endpoints.
struct OptionContract {
    std::string id;
    std::string symbol;
    OptionStatus status{OptionStatus::ACTIVE};
    bool tradable{false};
    std::string underlying_symbol;
    std::string expiration_date;
    std::string strike_price;
    OptionType type{OptionType::CALL};
    OptionStyle style{OptionStyle::AMERICAN};
    std::optional<std::string> root_symbol{};
    std::optional<OptionExchange> exchange{};
    std::optional<OptionStyle> exercise_style{};
    std::optional<std::string> multiplier{};
    std::optional<std::uint64_t> open_interest{};
    std::optional<std::string> open_interest_date{};
    std::optional<Money> close_price{};
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
    std::optional<OptionStatus> status{};
    std::optional<std::string> expiry{};
    std::optional<OptionType> type{};
    std::optional<OptionStyle> style{};
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
    std::optional<Money> theoretical_price{};
    std::optional<Money> underlying_price{};
    std::optional<Money> breakeven_price{};
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

std::string to_string(OptionType type);
std::string to_string(OptionStyle style);
std::string to_string(OptionStatus status);
std::string to_string(OptionExchange exchange);

OptionType option_type_from_string(std::string const& value);
OptionStyle option_style_from_string(std::string const& value);
OptionStatus option_status_from_string(std::string const& value);
OptionExchange option_exchange_from_string(std::string const& value);

} // namespace alpaca
