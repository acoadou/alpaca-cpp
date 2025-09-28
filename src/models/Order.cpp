#include "alpaca/models/Order.hpp"

#include <algorithm>
#include <cctype>
#include <type_traits>
#include <utility>
#include <vector>

#include "alpaca/Exceptions.hpp"
namespace alpaca {
namespace {

std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

template <typename Request> Json build_new_order_payload(Request const& request) {
    Json j{
        {"symbol",        request.symbol                  },
        {"side",          to_string(request.side)         },
        {"type",          to_string(request.type)         },
        {"time_in_force", to_string(request.time_in_force)}
    };

    if (request.quantity.has_value()) {
        j["qty"] = *request.quantity;
    }
    if (request.notional.has_value()) {
        j["notional"] = *request.notional;
    }
    if (request.limit_price.has_value()) {
        j["limit_price"] = *request.limit_price;
    }
    if (request.stop_price.has_value()) {
        j["stop_price"] = *request.stop_price;
    }
    if (request.trail_price.has_value()) {
        j["trail_price"] = *request.trail_price;
    }
    if (request.trail_percent.has_value()) {
        j["trail_percent"] = *request.trail_percent;
    }
    if (request.high_water_mark.has_value()) {
        j["high_water_mark"] = *request.high_water_mark;
    }
    if (request.client_order_id.has_value()) {
        j["client_order_id"] = *request.client_order_id;
    }
    if (request.order_class.has_value()) {
        j["order_class"] = to_string(*request.order_class);
    }
    if (request.take_profit.has_value()) {
        Json take_profit_json;
        to_json(take_profit_json, *request.take_profit);
        j["take_profit"] = std::move(take_profit_json);
    }
    if (request.stop_loss.has_value()) {
        Json stop_loss_json;
        to_json(stop_loss_json, *request.stop_loss);
        j["stop_loss"] = std::move(stop_loss_json);
    }

    if constexpr (requires { request.position_intent; }) {
        if (request.position_intent.has_value()) {
            j["position_intent"] = to_string(*request.position_intent);
        }
    }
    if constexpr (requires { request.legs; }) {
        if (!request.legs.empty()) {
            Json legs_json = Json::array();
            for (auto const& leg : request.legs) {
                Json leg_json = {
                    {"symbol",          leg.symbol           },
                    {"ratio",           leg.ratio            },
                    {"side",            to_string(leg.side)  },
                    {"position_intent", to_string(leg.intent)}
                };
                legs_json.push_back(std::move(leg_json));
            }
            j["legs"] = std::move(legs_json);
        }
    }

    if constexpr (requires { request.extended_hours; }) {
        if (request.extended_hours) {
            j["extended_hours"] = request.extended_hours;
        }
    }

    if constexpr (std::is_base_of_v<NewMultiAssetOrderRequest, Request>) {
        auto const& multi = static_cast<NewMultiAssetOrderRequest const&>(request);
        if (multi.base_symbol.has_value()) {
            j["base_symbol"] = *multi.base_symbol;
        }
        if (multi.quote_symbol.has_value()) {
            j["quote_symbol"] = *multi.quote_symbol;
        }
        if (multi.notional_currency.has_value()) {
            j["notional_currency"] = *multi.notional_currency;
        }
        if (multi.venue.has_value()) {
            j["venue"] = *multi.venue;
        }
        if (multi.routing_strategy.has_value()) {
            j["routing_strategy"] = *multi.routing_strategy;
        }
        if (multi.post_only.has_value()) {
            j["post_only"] = *multi.post_only;
        }
        if (multi.reduce_only.has_value()) {
            j["reduce_only"] = *multi.reduce_only;
        }
    }

    if constexpr (std::is_base_of_v<NewOtcOrderRequest, Request>) {
        auto const& otc = static_cast<NewOtcOrderRequest const&>(request);
        if (otc.counterparty.has_value()) {
            j["counterparty"] = *otc.counterparty;
        }
        if (otc.quote_id.has_value()) {
            j["quote_id"] = *otc.quote_id;
        }
        if (otc.settlement_date.has_value()) {
            j["settlement_date"] = *otc.settlement_date;
        }
    }

    return j;
}

template <typename Request> Json build_replace_order_payload(Request const& request) {
    Json j{};
    if (request.quantity.has_value()) {
        j["qty"] = *request.quantity;
    }
    if (request.time_in_force.has_value()) {
        j["time_in_force"] = *request.time_in_force;
    }
    if (request.limit_price.has_value()) {
        j["limit_price"] = *request.limit_price;
    }
    if (request.stop_price.has_value()) {
        j["stop_price"] = *request.stop_price;
    }
    if (request.extended_hours.has_value()) {
        j["extended_hours"] = *request.extended_hours;
    }
    if (request.client_order_id.has_value()) {
        j["client_order_id"] = *request.client_order_id;
    }

    if constexpr (std::is_base_of_v<ReplaceMultiAssetOrderRequest, Request>) {
        auto const& multi = static_cast<ReplaceMultiAssetOrderRequest const&>(request);
        if (multi.quote_symbol.has_value()) {
            j["quote_symbol"] = *multi.quote_symbol;
        }
        if (multi.venue.has_value()) {
            j["venue"] = *multi.venue;
        }
        if (multi.routing_strategy.has_value()) {
            j["routing_strategy"] = *multi.routing_strategy;
        }
        if (multi.post_only.has_value()) {
            j["post_only"] = *multi.post_only;
        }
        if (multi.reduce_only.has_value()) {
            j["reduce_only"] = *multi.reduce_only;
        }
    }

    if constexpr (std::is_base_of_v<ReplaceOtcOrderRequest, Request>) {
        auto const& otc = static_cast<ReplaceOtcOrderRequest const&>(request);
        if (otc.counterparty.has_value()) {
            j["counterparty"] = *otc.counterparty;
        }
        if (otc.settlement_date.has_value()) {
            j["settlement_date"] = *otc.settlement_date;
        }
    }

    return j;
}

void append_timestamp(QueryParams& params, std::string const& key, std::optional<Timestamp> const& value) {
    if (value.has_value()) {
        params.emplace_back(key, format_timestamp(*value));
    }
}

void append_symbols(QueryParams& params, std::vector<std::string> const& symbols) {
    if (symbols.empty()) {
        return;
    }
    params.emplace_back("symbols", join_csv(symbols));
}

} // namespace

std::string to_string(OrderStatus status) {
    switch (status) {
    case OrderStatus::NEW:
        return "new";
    case OrderStatus::PARTIALLY_FILLED:
        return "partially_filled";
    case OrderStatus::FILLED:
        return "filled";
    case OrderStatus::DONE_FOR_DAY:
        return "done_for_day";
    case OrderStatus::CANCELED:
        return "canceled";
    case OrderStatus::EXPIRED:
        return "expired";
    case OrderStatus::REPLACED:
        return "replaced";
    case OrderStatus::PENDING_CANCEL:
        return "pending_cancel";
    case OrderStatus::PENDING_REPLACE:
        return "pending_replace";
    case OrderStatus::ACCEPTED:
        return "accepted";
    case OrderStatus::PENDING_NEW:
        return "pending_new";
    case OrderStatus::ACCEPTED_FOR_BIDDING:
        return "accepted_for_bidding";
    case OrderStatus::STOPPED:
        return "stopped";
    case OrderStatus::REJECTED:
        return "rejected";
    case OrderStatus::SUSPENDED:
        return "suspended";
    case OrderStatus::CALCULATED:
        return "calculated";
    case OrderStatus::HELD:
        return "held";
    case OrderStatus::UNKNOWN:
        return "unknown";
    }
    throw std::invalid_argument("Unknown OrderStatus");
}

OrderStatus order_status_from_string(std::string const& value) {
    std::string const lower = to_lower_copy(value);
    if (lower == "new") {
        return OrderStatus::NEW;
    }
    if (lower == "partially_filled") {
        return OrderStatus::PARTIALLY_FILLED;
    }
    if (lower == "filled") {
        return OrderStatus::FILLED;
    }
    if (lower == "done_for_day") {
        return OrderStatus::DONE_FOR_DAY;
    }
    if (lower == "canceled" || lower == "cancelled") {
        return OrderStatus::CANCELED;
    }
    if (lower == "expired") {
        return OrderStatus::EXPIRED;
    }
    if (lower == "replaced") {
        return OrderStatus::REPLACED;
    }
    if (lower == "pending_cancel") {
        return OrderStatus::PENDING_CANCEL;
    }
    if (lower == "pending_replace") {
        return OrderStatus::PENDING_REPLACE;
    }
    if (lower == "accepted") {
        return OrderStatus::ACCEPTED;
    }
    if (lower == "pending_new") {
        return OrderStatus::PENDING_NEW;
    }
    if (lower == "accepted_for_bidding") {
        return OrderStatus::ACCEPTED_FOR_BIDDING;
    }
    if (lower == "stopped") {
        return OrderStatus::STOPPED;
    }
    if (lower == "rejected") {
        return OrderStatus::REJECTED;
    }
    if (lower == "suspended") {
        return OrderStatus::SUSPENDED;
    }
    if (lower == "calculated") {
        return OrderStatus::CALCULATED;
    }
    if (lower == "held") {
        return OrderStatus::HELD;
    }
    return OrderStatus::UNKNOWN;
}

std::string to_string(OrderStatusFilter status) {
    switch (status) {
    case OrderStatusFilter::OPEN:
        return "open";
    case OrderStatusFilter::CLOSED:
        return "closed";
    case OrderStatusFilter::ALL:
        return "all";
    }
    throw InvalidArgumentException("order_status_filter", "Unknown OrderStatusFilter", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                             },
                                       {"value",   std::to_string(static_cast<int>(status))}
    });
}

std::string to_string(PositionIntent intent) {
    switch (intent) {
    case PositionIntent::OPENING:
        return "opening";
    case PositionIntent::CLOSING:
        return "closing";
    case PositionIntent::AUTOMATIC:
        return "automatic";
    }
    throw InvalidArgumentException("position_intent", "Unknown PositionIntent", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                             },
                                       {"value",   std::to_string(static_cast<int>(intent))}
    });
}

void to_json(Json& j, NewOrderRequest const& request) {
    j = build_new_order_payload(request);
}

void to_json(Json& j, NewCryptoOrderRequest const& request) {
    j = build_new_order_payload(request);
}

void to_json(Json& j, NewOtcOrderRequest const& request) {
    j = build_new_order_payload(request);
}

void from_json(Json const& j, Order& order) {
    j.at("id").get_to(order.id);
    order.asset_id = j.value("asset_id", std::string{});
    order.client_order_id = j.value("client_order_id", std::string{});
    order.account_id = j.value("account_id", std::string{});
    order.created_at = parse_timestamp(j.at("created_at").get<std::string>());
    order.updated_at = parse_timestamp_field(j, "updated_at");
    order.submitted_at = parse_timestamp_field(j, "submitted_at");
    order.filled_at = parse_timestamp_field(j, "filled_at");
    order.expired_at = parse_timestamp_field(j, "expired_at");
    order.canceled_at = parse_timestamp_field(j, "canceled_at");
    order.failed_at = parse_timestamp_field(j, "failed_at");
    order.replaced_at = parse_timestamp_field(j, "replaced_at");
    order.replaced_by = j.value("replaced_by", std::string{});
    order.replaces = j.value("replaces", std::string{});
    order.symbol = j.value("symbol", std::string{});
    order.asset_class = j.value("asset_class", std::string{});
    if (j.contains("side") && !j.at("side").is_null()) {
        order.side = order_side_from_string(j.at("side").get<std::string>());
    }
    if (j.contains("type") && !j.at("type").is_null()) {
        order.type = order_type_from_string(j.at("type").get<std::string>());
    }
    if (j.contains("time_in_force") && !j.at("time_in_force").is_null()) {
        order.time_in_force = time_in_force_from_string(j.at("time_in_force").get<std::string>());
    }
    if (j.contains("order_class") && !j.at("order_class").is_null()) {
        order.order_class = order_class_from_string(j.at("order_class").get<std::string>());
    } else {
        order.order_class.reset();
    }
    if (j.contains("status") && !j.at("status").is_null()) {
        order.status = order_status_from_string(j.at("status").get<std::string>());
    } else {
        order.status = OrderStatus::UNKNOWN;
    }
    if (j.contains("qty") && !j.at("qty").is_null()) {
        order.qty = j.at("qty").get<std::string>();
    } else {
        order.qty.reset();
    }
    if (j.contains("notional") && !j.at("notional").is_null()) {
        order.notional = j.at("notional").get<std::string>();
    } else {
        order.notional.reset();
    }
    if (j.contains("filled_qty") && !j.at("filled_qty").is_null()) {
        order.filled_qty = j.at("filled_qty").get<std::string>();
    } else {
        order.filled_qty.reset();
    }
    if (j.contains("filled_avg_price") && !j.at("filled_avg_price").is_null()) {
        order.filled_avg_price = j.at("filled_avg_price").get<std::string>();
    } else {
        order.filled_avg_price.reset();
    }
    if (j.contains("limit_price") && !j.at("limit_price").is_null()) {
        order.limit_price = j.at("limit_price").get<std::string>();
    } else {
        order.limit_price.reset();
    }
    if (j.contains("stop_price") && !j.at("stop_price").is_null()) {
        order.stop_price = j.at("stop_price").get<std::string>();
    } else {
        order.stop_price.reset();
    }
    if (j.contains("trail_price") && !j.at("trail_price").is_null()) {
        order.trail_price = j.at("trail_price").get<std::string>();
    } else {
        order.trail_price.reset();
    }
    if (j.contains("trail_percent") && !j.at("trail_percent").is_null()) {
        order.trail_percent = j.at("trail_percent").get<std::string>();
    } else {
        order.trail_percent.reset();
    }
    if (j.contains("high_water_mark") && !j.at("high_water_mark").is_null()) {
        order.high_water_mark = j.at("high_water_mark").get<std::string>();
    } else if (j.contains("hwm") && !j.at("hwm").is_null()) {
        order.high_water_mark = j.at("hwm").get<std::string>();
    } else {
        order.high_water_mark.reset();
    }
    order.extended_hours = j.value("extended_hours", false);
    if (j.contains("base_symbol") && !j.at("base_symbol").is_null()) {
        order.base_symbol = j.at("base_symbol").get<std::string>();
    } else {
        order.base_symbol.reset();
    }
    if (j.contains("quote_symbol") && !j.at("quote_symbol").is_null()) {
        order.quote_symbol = j.at("quote_symbol").get<std::string>();
    } else {
        order.quote_symbol.reset();
    }
    if (j.contains("notional_currency") && !j.at("notional_currency").is_null()) {
        order.notional_currency = j.at("notional_currency").get<std::string>();
    } else {
        order.notional_currency.reset();
    }
    if (j.contains("venue") && !j.at("venue").is_null()) {
        order.venue = j.at("venue").get<std::string>();
    } else {
        order.venue.reset();
    }
    if (j.contains("routing_strategy") && !j.at("routing_strategy").is_null()) {
        order.routing_strategy = j.at("routing_strategy").get<std::string>();
    } else {
        order.routing_strategy.reset();
    }
    if (j.contains("post_only") && !j.at("post_only").is_null()) {
        order.post_only = j.at("post_only").get<bool>();
    } else {
        order.post_only.reset();
    }
    if (j.contains("reduce_only") && !j.at("reduce_only").is_null()) {
        order.reduce_only = j.at("reduce_only").get<bool>();
    } else {
        order.reduce_only.reset();
    }
    if (j.contains("counterparty") && !j.at("counterparty").is_null()) {
        order.counterparty = j.at("counterparty").get<std::string>();
    } else {
        order.counterparty.reset();
    }
    if (j.contains("quote_id") && !j.at("quote_id").is_null()) {
        order.quote_id = j.at("quote_id").get<std::string>();
    } else {
        order.quote_id.reset();
    }
    if (j.contains("settlement_date") && !j.at("settlement_date").is_null()) {
        order.settlement_date = j.at("settlement_date").get<std::string>();
    } else {
        order.settlement_date.reset();
    }
    if (j.contains("legs") && !j.at("legs").is_null()) {
        order.legs = j.at("legs").get<std::vector<Order>>();
    } else {
        order.legs.clear();
    }
}

void to_json(Json& j, ReplaceOrderRequest const& request) {
    j = build_replace_order_payload(request);
}

void to_json(Json& j, ReplaceCryptoOrderRequest const& request) {
    j = build_replace_order_payload(request);
}

void to_json(Json& j, ReplaceOtcOrderRequest const& request) {
    j = build_replace_order_payload(request);
}

QueryParams ListOrdersRequest::to_query_params() const {
    QueryParams params;
    if (status.has_value()) {
        params.emplace_back("status", to_string(*status));
    }
    if (limit.has_value()) {
        params.emplace_back("limit", std::to_string(*limit));
    }
    append_timestamp(params, "after", after);
    append_timestamp(params, "until", until);
    if (direction.has_value()) {
        params.emplace_back("direction", to_string(*direction));
    }
    if (side.has_value()) {
        params.emplace_back("side", to_string(*side));
    }
    if (nested.has_value()) {
        params.emplace_back("nested", *nested ? "true" : "false");
    }
    append_symbols(params, symbols);
    if (asset_class.has_value()) {
        params.emplace_back("asset_class", to_string(*asset_class));
    }
    if (venue.has_value()) {
        params.emplace_back("venue", *venue);
    }
    return params;
}

} // namespace alpaca
