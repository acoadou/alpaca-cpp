#include "alpaca/models/Option.hpp"

#include <cstdint>
#include <utility>

namespace alpaca {
namespace {
std::string extract_string(Json const& value) {
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_number_integer()) {
        return std::to_string(value.get<std::int64_t>());
    }
    if (value.is_number_unsigned()) {
        return std::to_string(value.get<std::uint64_t>());
    }
    if (value.is_number_float()) {
        auto const numeric = value.get<double>();
        auto result = std::to_string(numeric);
        while (!result.empty() && result.back() == '0') {
            result.pop_back();
        }
        if (!result.empty() && result.back() == '.') {
            result.pop_back();
        }
        return result;
    }
    return value.dump();
}

std::optional<double> parse_optional_double(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    if (j.at(key).is_string()) {
        auto const as_string = j.at(key).get<std::string>();
        if (as_string.empty()) {
            return std::nullopt;
        }
        return std::stod(as_string);
    }
    return j.at(key).get<double>();
}

std::optional<std::uint64_t> parse_optional_uint64(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    if (j.at(key).is_string()) {
        auto const value = j.at(key).get<std::string>();
        if (value.empty()) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(std::stoull(value));
    }
    if (j.at(key).is_number_unsigned()) {
        return j.at(key).get<std::uint64_t>();
    }
    if (j.at(key).is_number_integer()) {
        auto const number = j.at(key).get<std::int64_t>();
        if (number < 0) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(number);
    }
    return std::nullopt;
}

std::optional<std::string> optional_string_from_any(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    auto const& value = j.at(key);
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_number_integer()) {
        return std::to_string(value.get<std::int64_t>());
    }
    if (value.is_number_unsigned()) {
        return std::to_string(value.get<std::uint64_t>());
    }
    if (value.is_number_float()) {
        return std::to_string(value.get<double>());
    }
    return value.dump();
}

void append_if_present(QueryParams& params, std::string const& key, std::optional<std::string> const& value) {
    if (value.has_value()) {
        params.emplace_back(key, *value);
    }
}

} // namespace

void from_json(Json const& j, OptionPosition& position) {
    j.at("asset_id").get_to(position.asset_id);
    position.symbol = j.value("symbol", std::string{});
    position.exchange = j.value("exchange", std::string{});
    position.asset_class = j.value("asset_class", std::string{});
    position.account_id = j.value("account_id", std::string{});
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
    position.side = j.value("side", std::string{});

    if (j.contains("contract_multiplier") && !j.at("contract_multiplier").is_null()) {
        position.contract_multiplier = extract_string(j.at("contract_multiplier"));
    } else {
        position.contract_multiplier.reset();
    }
    if (j.contains("expiry") && !j.at("expiry").is_null()) {
        position.expiry = j.at("expiry").get<std::string>();
    } else {
        position.expiry.reset();
    }
    if (j.contains("strike_price") && !j.at("strike_price").is_null()) {
        position.strike_price = extract_string(j.at("strike_price"));
    } else {
        position.strike_price.reset();
    }
    if (j.contains("style") && !j.at("style").is_null()) {
        position.style = j.at("style").get<std::string>();
    } else {
        position.style.reset();
    }
    if (j.contains("type") && !j.at("type").is_null()) {
        position.type = j.at("type").get<std::string>();
    } else {
        position.type.reset();
    }
    if (j.contains("underlying_symbol") && !j.at("underlying_symbol").is_null()) {
        position.underlying_symbol = j.at("underlying_symbol").get<std::string>();
    } else {
        position.underlying_symbol.reset();
    }
}

void from_json(Json const& j, OptionContract& contract) {
    j.at("id").get_to(contract.id);
    contract.symbol = j.value("symbol", std::string{});
    contract.status = j.value("status", std::string{});
    contract.tradable = j.value("tradable", false);
    contract.underlying_symbol = j.value("underlying_symbol", std::string{});
    contract.expiration_date = j.value("expiration_date", std::string{});
    if (j.contains("strike_price") && !j.at("strike_price").is_null()) {
        contract.strike_price = extract_string(j.at("strike_price"));
    } else {
        contract.strike_price.clear();
    }
    contract.type = j.value("type", std::string{});
    contract.style = j.value("style", std::string{});
    if (j.contains("root_symbol") && !j.at("root_symbol").is_null()) {
        contract.root_symbol = j.at("root_symbol").get<std::string>();
    } else {
        contract.root_symbol.reset();
    }
    if (j.contains("exchange") && !j.at("exchange").is_null()) {
        contract.exchange = j.at("exchange").get<std::string>();
    } else {
        contract.exchange.reset();
    }
    if (j.contains("exercise_style") && !j.at("exercise_style").is_null()) {
        contract.exercise_style = j.at("exercise_style").get<std::string>();
    } else {
        contract.exercise_style.reset();
    }
    if (j.contains("multiplier") && !j.at("multiplier").is_null()) {
        contract.multiplier = extract_string(j.at("multiplier"));
    } else {
        contract.multiplier.reset();
    }
    contract.open_interest = parse_optional_uint64(j, "open_interest");
    contract.open_interest_date = optional_string_from_any(j, "open_interest_date");
    contract.close_price = parse_optional_double(j, "close_price");
    contract.contract_size = optional_string_from_any(j, "contract_size");
    contract.underlying_asset_id = optional_string_from_any(j, "underlying_asset_id");
}

void from_json(Json const& j, OptionContractsResponse& response) {
    if (j.contains("contracts") && j.at("contracts").is_array()) {
        response.contracts = j.at("contracts").get<std::vector<OptionContract>>();
    } else {
        response.contracts.clear();
    }
    if (j.contains("next_page_token") && !j.at("next_page_token").is_null()) {
        response.next_page_token = j.at("next_page_token").get<std::string>();
    } else {
        response.next_page_token.reset();
    }
}

QueryParams ListOptionContractsRequest::to_query_params() const {
    QueryParams params;
    if (!underlying_symbols.empty()) {
        params.emplace_back("underlying_symbols", join_csv(underlying_symbols));
    }
    append_if_present(params, "status", status);
    append_if_present(params, "expiry", expiry);
    append_if_present(params, "type", type);
    append_if_present(params, "style", style);
    append_if_present(params, "strike", strike);
    append_if_present(params, "strike_gte", strike_gte);
    append_if_present(params, "strike_lte", strike_lte);
    if (limit.has_value()) {
        params.emplace_back("limit", std::to_string(*limit));
    }
    if (direction.has_value()) {
        params.emplace_back("direction", to_string(*direction));
    }
    append_if_present(params, "page_token", page_token);
    return params;
}

void from_json(Json const& j, OptionGreeks& greeks) {
    greeks.delta = parse_optional_double(j, "delta");
    greeks.gamma = parse_optional_double(j, "gamma");
    greeks.theta = parse_optional_double(j, "theta");
    greeks.vega = parse_optional_double(j, "vega");
    greeks.rho = parse_optional_double(j, "rho");
}

void from_json(Json const& j, OptionRiskParameters& risk) {
    risk.implied_volatility = parse_optional_double(j, "implied_volatility");
    risk.theoretical_price = parse_optional_double(j, "theoretical_price");
    risk.underlying_price = parse_optional_double(j, "underlying_price");
    risk.breakeven_price = parse_optional_double(j, "breakeven_price");
}

void from_json(Json const& j, OptionStrategyLeg& leg) {
    j.at("symbol").get_to(leg.symbol);
    if (j.contains("side") && !j.at("side").is_null()) {
        leg.side = order_side_from_string(j.at("side").get<std::string>());
    }
    leg.ratio = j.value("ratio", 1);
}

void from_json(Json const& j, OptionAnalytics& analytics) {
    j.at("symbol").get_to(analytics.symbol);
    if (j.contains("greeks") && !j.at("greeks").is_null()) {
        analytics.greeks = j.at("greeks").get<OptionGreeks>();
    } else {
        analytics.greeks.reset();
    }
    if (j.contains("risk_parameters") && !j.at("risk_parameters").is_null()) {
        analytics.risk_parameters = j.at("risk_parameters").get<OptionRiskParameters>();
    } else {
        analytics.risk_parameters.reset();
    }
    analytics.implied_volatility = parse_optional_double(j, "implied_volatility");
    if (j.contains("legs") && !j.at("legs").is_null()) {
        analytics.legs = j.at("legs").get<std::vector<OptionStrategyLeg>>();
    } else {
        analytics.legs.clear();
    }
}

void from_json(Json const& j, OptionAnalyticsResponse& response) {
    if (j.contains("analytics") && j.at("analytics").is_array()) {
        response.analytics = j.at("analytics").get<std::vector<OptionAnalytics>>();
    } else {
        response.analytics.clear();
    }
    if (j.contains("next_page_token") && !j.at("next_page_token").is_null()) {
        response.next_page_token = j.at("next_page_token").get<std::string>();
    } else {
        response.next_page_token.reset();
    }
}

QueryParams ListOptionAnalyticsRequest::to_query_params() const {
    QueryParams params;
    if (!symbols.empty()) {
        params.emplace_back("symbols", join_csv(symbols));
    }
    append_if_present(params, "underlying_symbol", underlying_symbol);
    if (include_greeks.has_value()) {
        params.emplace_back("include_greeks", *include_greeks ? "true" : "false");
    }
    if (include_risk_parameters.has_value()) {
        params.emplace_back("include_risk_parameters", *include_risk_parameters ? "true" : "false");
    }
    if (limit.has_value()) {
        params.emplace_back("limit", std::to_string(*limit));
    }
    append_if_present(params, "page_token", page_token);
    return params;
}

} // namespace alpaca
