#include "alpaca/models/Option.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <utility>

#include "alpaca/Exceptions.hpp"

namespace alpaca {
namespace {
std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string to_upper_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::string normalize_exchange_code(std::string value) {
    for (auto& ch : value) {
        if (ch == '-' || ch == ' ') {
            ch = '_';
        }
    }
    return to_upper_copy(std::move(value));
}

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

std::optional<Money> parse_optional_money(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    if (j.at(key).is_string()) {
        auto const value = j.at(key).get<std::string>();
        if (value.empty()) {
            return std::nullopt;
        }
        return Money{value};
    }
    if (j.at(key).is_number_float()) {
        return Money{j.at(key).get<double>()};
    }
    if (j.at(key).is_number_integer()) {
        return Money{static_cast<double>(j.at(key).get<std::int64_t>())};
    }
    return std::nullopt;
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

std::string to_string(OptionType type) {
    switch (type) {
    case OptionType::CALL:
        return "call";
    case OptionType::PUT:
        return "put";
    }
    throw InvalidArgumentException("option_type", "Unknown OptionType", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                           },
                                       {"value",   std::to_string(static_cast<int>(type))}
    });
}

std::string to_string(OptionStyle style) {
    switch (style) {
    case OptionStyle::AMERICAN:
        return "american";
    case OptionStyle::EUROPEAN:
        return "european";
    }
    throw InvalidArgumentException("option_style", "Unknown OptionStyle", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                            },
                                       {"value",   std::to_string(static_cast<int>(style))}
    });
}

std::string to_string(OptionStatus status) {
    switch (status) {
    case OptionStatus::ACTIVE:
        return "active";
    case OptionStatus::HALTED:
        return "halted";
    case OptionStatus::INACTIVE:
        return "inactive";
    }
    throw InvalidArgumentException("option_status", "Unknown OptionStatus", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                             },
                                       {"value",   std::to_string(static_cast<int>(status))}
    });
}

std::string to_string(OptionExchange exchange) {
    switch (exchange) {
    case OptionExchange::AMEX:
        return "AMEX";
    case OptionExchange::ARCA:
        return "ARCA";
    case OptionExchange::BATS:
        return "BATS";
    case OptionExchange::BOX:
        return "BOX";
    case OptionExchange::BZX:
        return "BZX";
    case OptionExchange::C2:
        return "C2";
    case OptionExchange::CBOE:
        return "CBOE";
    case OptionExchange::EDGX:
        return "EDGX";
    case OptionExchange::GEMINI:
        return "GEMINI";
    case OptionExchange::ISE:
        return "ISE";
    case OptionExchange::ISE_MERCURY:
        return "ISE_MERCURY";
    case OptionExchange::MIAX:
        return "MIAX";
    case OptionExchange::MIAX_EMERALD:
        return "MIAX_EMERALD";
    case OptionExchange::MIAX_PEARL:
        return "MIAX_PEARL";
    case OptionExchange::NASDAQ:
        return "NASDAQ";
    case OptionExchange::NASDAQ_BX:
        return "NASDAQ_BX";
    case OptionExchange::NASDAQ_OMX:
        return "NASDAQ_OMX";
    case OptionExchange::NASDAQ_PHLX:
        return "NASDAQ_PHLX";
    case OptionExchange::NYSE:
        return "NYSE";
    case OptionExchange::NYSE_ARCA:
        return "NYSE_ARCA";
    case OptionExchange::OPRA:
        return "OPRA";
    }
    throw InvalidArgumentException(
    "option_exchange", "Unknown OptionExchange", ErrorCode::InvalidArgument,
    {
        {"context", "to_string"                               },
        {"value",   std::to_string(static_cast<int>(exchange))}
    });
}

OptionType option_type_from_string(std::string const& value) {
    auto const lower = to_lower_copy(value);
    if (lower == "call") {
        return OptionType::CALL;
    }
    if (lower == "put") {
        return OptionType::PUT;
    }
    throw InvalidArgumentException("option_type", "Unknown option type: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                    },
                                       {"context", "option_type_from_string"}
    });
}

OptionStyle option_style_from_string(std::string const& value) {
    auto const lower = to_lower_copy(value);
    if (lower == "american") {
        return OptionStyle::AMERICAN;
    }
    if (lower == "european") {
        return OptionStyle::EUROPEAN;
    }
    throw InvalidArgumentException("option_style", "Unknown option style: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                     },
                                       {"context", "option_style_from_string"}
    });
}

OptionStatus option_status_from_string(std::string const& value) {
    auto const lower = to_lower_copy(value);
    if (lower == "active") {
        return OptionStatus::ACTIVE;
    }
    if (lower == "halted") {
        return OptionStatus::HALTED;
    }
    if (lower == "inactive") {
        return OptionStatus::INACTIVE;
    }
    throw InvalidArgumentException("option_status", "Unknown option status: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                      },
                                       {"context", "option_status_from_string"}
    });
}

OptionExchange option_exchange_from_string(std::string const& value) {
    auto const normalized = normalize_exchange_code(value);
    if (normalized == "AMEX") {
        return OptionExchange::AMEX;
    }
    if (normalized == "ARCA") {
        return OptionExchange::ARCA;
    }
    if (normalized == "BATS") {
        return OptionExchange::BATS;
    }
    if (normalized == "BOX") {
        return OptionExchange::BOX;
    }
    if (normalized == "BZX") {
        return OptionExchange::BZX;
    }
    if (normalized == "C2") {
        return OptionExchange::C2;
    }
    if (normalized == "CBOE") {
        return OptionExchange::CBOE;
    }
    if (normalized == "EDGX") {
        return OptionExchange::EDGX;
    }
    if (normalized == "GEMINI") {
        return OptionExchange::GEMINI;
    }
    if (normalized == "ISE") {
        return OptionExchange::ISE;
    }
    if (normalized == "ISE_MERCURY") {
        return OptionExchange::ISE_MERCURY;
    }
    if (normalized == "MIAX") {
        return OptionExchange::MIAX;
    }
    if (normalized == "MIAX_EMERALD") {
        return OptionExchange::MIAX_EMERALD;
    }
    if (normalized == "MIAX_PEARL") {
        return OptionExchange::MIAX_PEARL;
    }
    if (normalized == "NASDAQ") {
        return OptionExchange::NASDAQ;
    }
    if (normalized == "NASDAQ_BX") {
        return OptionExchange::NASDAQ_BX;
    }
    if (normalized == "NASDAQ_OMX" || normalized == "NASDAQ_OMX_BX" || normalized == "NOM") {
        return OptionExchange::NASDAQ_OMX;
    }
    if (normalized == "NASDAQ_PHLX" || normalized == "PHLX") {
        return OptionExchange::NASDAQ_PHLX;
    }
    if (normalized == "NYSE") {
        return OptionExchange::NYSE;
    }
    if (normalized == "NYSE_ARCA") {
        return OptionExchange::NYSE_ARCA;
    }
    if (normalized == "OPRA") {
        return OptionExchange::OPRA;
    }
    throw InvalidArgumentException(
    "option_exchange", "Unknown option exchange: " + value, ErrorCode::InvalidArgument,
    {
        {"value",      value                        },
        {"normalized", normalized                   },
        {"context",    "option_exchange_from_string"}
    });
}

void from_json(Json const& j, OptionPosition& position) {
    j.at("asset_id").get_to(position.asset_id);
    position.symbol = j.value("symbol", std::string{});
    if (j.contains("exchange") && !j.at("exchange").is_null()) {
        position.exchange = option_exchange_from_string(j.at("exchange").get<std::string>());
    } else {
        position.exchange.reset();
    }
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
        position.style = option_style_from_string(j.at("style").get<std::string>());
    } else {
        position.style.reset();
    }
    if (j.contains("type") && !j.at("type").is_null()) {
        position.type = option_type_from_string(j.at("type").get<std::string>());
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
    if (j.contains("status") && !j.at("status").is_null()) {
        contract.status = option_status_from_string(j.at("status").get<std::string>());
    } else {
        contract.status = OptionStatus::ACTIVE;
    }
    contract.tradable = j.value("tradable", false);
    contract.underlying_symbol = j.value("underlying_symbol", std::string{});
    contract.expiration_date = j.value("expiration_date", std::string{});
    if (j.contains("strike_price") && !j.at("strike_price").is_null()) {
        contract.strike_price = extract_string(j.at("strike_price"));
    } else {
        contract.strike_price.clear();
    }
    if (j.contains("type") && !j.at("type").is_null()) {
        contract.type = option_type_from_string(j.at("type").get<std::string>());
    } else {
        contract.type = OptionType::CALL;
    }
    if (j.contains("style") && !j.at("style").is_null()) {
        contract.style = option_style_from_string(j.at("style").get<std::string>());
    } else {
        contract.style = OptionStyle::AMERICAN;
    }
    if (j.contains("root_symbol") && !j.at("root_symbol").is_null()) {
        contract.root_symbol = j.at("root_symbol").get<std::string>();
    } else {
        contract.root_symbol.reset();
    }
    if (j.contains("exchange") && !j.at("exchange").is_null()) {
        contract.exchange = option_exchange_from_string(j.at("exchange").get<std::string>());
    } else {
        contract.exchange.reset();
    }
    if (j.contains("exercise_style") && !j.at("exercise_style").is_null()) {
        contract.exercise_style = option_style_from_string(j.at("exercise_style").get<std::string>());
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
    contract.close_price = parse_optional_money(j, "close_price");
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
    if (status.has_value()) {
        params.emplace_back("status", to_string(*status));
    }
    append_if_present(params, "expiry", expiry);
    if (type.has_value()) {
        params.emplace_back("type", to_string(*type));
    }
    if (style.has_value()) {
        params.emplace_back("style", to_string(*style));
    }
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
    risk.theoretical_price = parse_optional_money(j, "theoretical_price");
    risk.underlying_price = parse_optional_money(j, "underlying_price");
    risk.breakeven_price = parse_optional_money(j, "breakeven_price");
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
