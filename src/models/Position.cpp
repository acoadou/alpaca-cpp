#include "alpaca/models/Position.hpp"

#include <variant>

#include "alpaca/models/Order.hpp"

namespace alpaca {

void from_json(Json const& j, Position& position) {
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
        params.emplace_back("limit_price", limit_price->to_string());
    }
    if (stop_price.has_value()) {
        params.emplace_back("stop_price", stop_price->to_string());
    }
    return params;
}

QueryParams CloseAllPositionsRequest::to_query_params() const {
    QueryParams params;
    if (cancel_orders.has_value()) {
        params.emplace_back("cancel_orders", *cancel_orders ? "true" : "false");
    }
    return params;
}

void from_json(Json const& j, FailedClosePositionDetails& details) {
    if (j.contains("code") && !j.at("code").is_null()) {
        details.code = j.at("code").get<int>();
    } else {
        details.code.reset();
    }
    if (j.contains("message") && !j.at("message").is_null()) {
        details.message = j.at("message").get<std::string>();
    } else {
        details.message.reset();
    }
    if (j.contains("available") && !j.at("available").is_null()) {
        details.available = j.at("available").get<double>();
    } else {
        details.available.reset();
    }
    if (j.contains("existing_qty") && !j.at("existing_qty").is_null()) {
        details.existing_qty = j.at("existing_qty").get<double>();
    } else {
        details.existing_qty.reset();
    }
    if (j.contains("held_for_orders") && !j.at("held_for_orders").is_null()) {
        details.held_for_orders = j.at("held_for_orders").get<double>();
    } else {
        details.held_for_orders.reset();
    }
    if (j.contains("symbol") && !j.at("symbol").is_null()) {
        details.symbol = j.at("symbol").get<std::string>();
    } else {
        details.symbol.reset();
    }
}

void from_json(Json const& j, ClosePositionResponse& response) {
    if (j.contains("order_id") && !j.at("order_id").is_null()) {
        response.order_id = j.at("order_id").get<std::string>();
    } else {
        response.order_id.reset();
    }
    if (j.contains("status") && !j.at("status").is_null()) {
        response.status = j.at("status").get<int>();
    } else {
        response.status.reset();
    }
    if (j.contains("symbol") && !j.at("symbol").is_null()) {
        response.symbol = j.at("symbol").get<std::string>();
    } else {
        response.symbol.reset();
    }

    response.body = std::monostate{};
    auto const body_it = j.find("body");
    if (body_it != j.end() && !body_it->is_null()) {
        if (body_it->is_object() && body_it->contains("id")) {
            response.body = body_it->get<Order>();
        } else if (body_it->is_object()) {
            response.body = body_it->get<FailedClosePositionDetails>();
        }
    }
}

} // namespace alpaca
