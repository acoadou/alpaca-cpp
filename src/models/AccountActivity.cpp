#include "alpaca/models/AccountActivity.hpp"

namespace alpaca {

void from_json(const Json& j, AccountActivity& activity) {
  activity.id = j.value("id", std::string{});
  activity.activity_type = j.value("activity_type", std::string{});
  activity.transaction_time = parse_timestamp_field(j, "transaction_time");
  activity.type = j.value("type", std::string{});
  activity.symbol = j.value("symbol", std::string{});
  activity.side = j.value("side", std::string{});
  activity.qty = j.value("qty", std::string{});
  activity.price = j.value("price", std::string{});
  activity.net_amount = j.value("net_amount", std::string{});
  activity.per_share_amount = j.value("per_share_amount", std::string{});
  activity.cumulative_qty = j.value("cumulative_qty", std::string{});
  activity.leaves_qty = j.value("leaves_qty", std::string{});
  if (j.contains("order_id") && !j.at("order_id").is_null()) {
    activity.order_id = j.at("order_id").get<std::string>();
  } else {
    activity.order_id.reset();
  }
  if (j.contains("order_status") && !j.at("order_status").is_null()) {
    activity.order_status = j.at("order_status").get<std::string>();
  } else {
    activity.order_status.reset();
  }
}

QueryParams AccountActivitiesRequest::to_query_params() const {
  QueryParams params;
  if (!activity_types.empty()) {
    params.emplace_back("activity_types", join_csv(activity_types));
  }
  if (date.has_value()) {
    params.emplace_back("date", format_calendar_date(*date));
  }
  if (until.has_value()) {
    params.emplace_back("until", format_timestamp(*until));
  }
  if (after.has_value()) {
    params.emplace_back("after", format_timestamp(*after));
  }
  if (direction.has_value()) {
    params.emplace_back("direction", to_string(*direction));
  }
  if (page_size.has_value()) {
    params.emplace_back("page_size", std::to_string(*page_size));
  }
  if (page_token.has_value()) {
    params.emplace_back("page_token", *page_token);
  }
  return params;
}

}  // namespace alpaca
