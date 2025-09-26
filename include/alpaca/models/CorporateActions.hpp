#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Represents a corporate action announcement returned by the Alpaca API.
struct CorporateActionAnnouncement {
  std::string id;
  std::string corporate_action_id;
  std::string type;
  std::string sub_type;
  std::string initiating_symbol;
  std::optional<std::string> initiating_original_cusip{};
  std::optional<std::string> target_symbol{};
  std::optional<std::string> target_original_cusip{};
  std::optional<std::string> declaration_date{};
  std::optional<std::string> record_date{};
  std::optional<std::string> payable_date{};
  std::optional<std::string> ex_date{};
  std::optional<std::string> cash{};
  std::optional<std::string> old_rate{};
  std::optional<std::string> new_rate{};
  std::optional<std::string> split_ratio{};
  Json raw{};
};

void from_json(const Json& j, CorporateActionAnnouncement& announcement);

/// Paginated wrapper for announcement responses.
struct CorporateActionAnnouncementsResponse {
  std::vector<CorporateActionAnnouncement> announcements{};
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, CorporateActionAnnouncementsResponse& response);

/// Represents a corporate action event returned by the Alpaca API.
struct CorporateActionEvent {
  std::string id;
  std::string corporate_action_id;
  std::string type;
  std::string sub_type;
  std::string symbol;
  std::optional<std::string> status{};
  std::optional<std::string> effective_date{};
  std::optional<Timestamp> created_at{};
  std::optional<Timestamp> updated_at{};
  Json raw{};
};

void from_json(const Json& j, CorporateActionEvent& event);

/// Paginated wrapper for corporate action events.
struct CorporateActionEventsResponse {
  std::vector<CorporateActionEvent> events{};
  std::optional<std::string> next_page_token{};
};

void from_json(const Json& j, CorporateActionEventsResponse& response);

}  // namespace alpaca
