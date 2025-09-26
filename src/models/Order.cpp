#include "alpaca/models/Order.hpp"

#include <stdexcept>
#include <utility>

namespace alpaca {

std::string to_string(OrderStatusFilter status) {
  switch (status) {
    case OrderStatusFilter::OPEN:
      return "open";
    case OrderStatusFilter::CLOSED:
      return "closed";
    case OrderStatusFilter::ALL:
      return "all";
  }
  throw std::invalid_argument("Unknown OrderStatusFilter");
}

void to_json(Json& j, const NewOrderRequest& request) {
  j = Json{{"symbol", request.symbol},
           {"side", to_string(request.side)},
           {"type", to_string(request.type)},
           {"time_in_force", to_string(request.time_in_force)}};
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
  if (request.client_order_id.has_value()) {
    j["client_order_id"] = *request.client_order_id;
  }
  if (request.extended_hours) {
    j["extended_hours"] = request.extended_hours;
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
}

void from_json(const Json& j, Order& order) {
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
  order.status = j.value("status", std::string{});
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
  order.extended_hours = j.value("extended_hours", false);
  if (j.contains("legs") && !j.at("legs").is_null()) {
    order.legs = j.at("legs").get<std::vector<Order>>();
  } else {
    order.legs.clear();
  }
}

void to_json(Json& j, const ReplaceOrderRequest& request) {
  j = Json{};
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
}

namespace {
void append_timestamp(QueryParams& params, const std::string& key, const std::optional<Timestamp>& value) {
  if (value.has_value()) {
    params.emplace_back(key, format_timestamp(*value));
  }
}

void append_symbols(QueryParams& params, const std::vector<std::string>& symbols) {
  if (symbols.empty()) {
    return;
  }
  params.emplace_back("symbols", join_csv(symbols));
}
}  // namespace

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
  if (nested.has_value()) {
    params.emplace_back("nested", *nested ? "true" : "false");
  }
  append_symbols(params, symbols);
  return params;
}

}  // namespace alpaca
