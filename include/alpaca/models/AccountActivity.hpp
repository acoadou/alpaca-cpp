#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Represents a single account activity entry such as a fill or fee.
struct AccountActivity {
  std::string id;
  std::string activity_type;
  std::optional<Timestamp> transaction_time{};
  std::string type;
  std::string symbol;
  std::string side;
  std::string qty;
  std::string price;
  std::string net_amount;
  std::string per_share_amount;
  std::string cumulative_qty;
  std::string leaves_qty;
  std::optional<std::string> order_id;
  std::optional<std::string> order_status;
};

void from_json(const Json& j, AccountActivity& activity);

/// Request parameters for the account activities endpoint.
struct AccountActivitiesRequest {
  std::vector<std::string> activity_types{};
  std::optional<std::chrono::sys_days> date{};
  std::optional<Timestamp> until{};
  std::optional<Timestamp> after{};
  std::optional<SortDirection> direction{};
  std::optional<int> page_size;
  std::optional<std::string> page_token;

  [[nodiscard]] QueryParams to_query_params() const;
};

}  // namespace alpaca
