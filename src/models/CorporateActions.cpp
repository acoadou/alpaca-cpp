#include "alpaca/models/CorporateActions.hpp"

namespace alpaca {
namespace {
std::optional<std::string> optional_string(const Json& j, const char* key) {
  if (!j.contains(key) || j.at(key).is_null()) {
    return std::nullopt;
  }
  return j.at(key).get<std::string>();
}

std::optional<Timestamp> optional_timestamp(const Json& j, const char* key) {
  if (!j.contains(key) || j.at(key).is_null()) {
    return std::nullopt;
  }
  return parse_timestamp(j.at(key).get<std::string>());
}
}  // namespace

void from_json(const Json& j, CorporateActionAnnouncement& announcement) {
  announcement.raw = j;
  if (j.contains("id")) {
    announcement.id = j.at("id").get<std::string>();
  }
  if (j.contains("corporate_action_id")) {
    announcement.corporate_action_id = j.at("corporate_action_id").get<std::string>();
  } else if (j.contains("ca_id")) {
    announcement.corporate_action_id = j.at("ca_id").get<std::string>();
  }
  if (j.contains("ca_type")) {
    announcement.type = j.at("ca_type").get<std::string>();
  } else if (j.contains("type")) {
    announcement.type = j.at("type").get<std::string>();
  }
  if (j.contains("ca_sub_type")) {
    announcement.sub_type = j.at("ca_sub_type").get<std::string>();
  } else if (j.contains("sub_type")) {
    announcement.sub_type = j.at("sub_type").get<std::string>();
  }
  if (j.contains("initiating_symbol")) {
    announcement.initiating_symbol = j.at("initiating_symbol").get<std::string>();
  }
  announcement.initiating_original_cusip = optional_string(j, "initiating_original_cusip");
  announcement.target_symbol = optional_string(j, "target_symbol");
  announcement.target_original_cusip = optional_string(j, "target_original_cusip");
  announcement.declaration_date = optional_string(j, "declaration_date");
  announcement.record_date = optional_string(j, "record_date");
  announcement.payable_date = optional_string(j, "payable_date");
  announcement.ex_date = optional_string(j, "ex_date");
  announcement.cash = optional_string(j, "cash");
  announcement.old_rate = optional_string(j, "old_rate");
  announcement.new_rate = optional_string(j, "new_rate");
  announcement.split_ratio = optional_string(j, "split_ratio");
}

void from_json(const Json& j, CorporateActionAnnouncementsResponse& response) {
  if (j.contains("announcements") && j.at("announcements").is_array()) {
    response.announcements = j.at("announcements").get<std::vector<CorporateActionAnnouncement>>();
  } else {
    response.announcements.clear();
  }
  if (j.contains("next_page_token") && !j.at("next_page_token").is_null()) {
    response.next_page_token = j.at("next_page_token").get<std::string>();
  } else {
    response.next_page_token = std::nullopt;
  }
}

void from_json(const Json& j, CorporateActionEvent& event) {
  event.raw = j;
  j.at("id").get_to(event.id);
  if (j.contains("corporate_action_id")) {
    event.corporate_action_id = j.at("corporate_action_id").get<std::string>();
  } else if (j.contains("ca_id")) {
    event.corporate_action_id = j.at("ca_id").get<std::string>();
  }
  if (j.contains("ca_type")) {
    event.type = j.at("ca_type").get<std::string>();
  } else {
    event.type = j.at("type").get<std::string>();
  }
  if (j.contains("ca_sub_type")) {
    event.sub_type = j.at("ca_sub_type").get<std::string>();
  } else {
    event.sub_type = j.at("sub_type").get<std::string>();
  }
  if (j.contains("symbol")) {
    event.symbol = j.at("symbol").get<std::string>();
  } else if (j.contains("initiating_symbol")) {
    event.symbol = j.at("initiating_symbol").get<std::string>();
  }
  event.status = optional_string(j, "status");
  event.effective_date = optional_string(j, "effective_date");
  event.created_at = optional_timestamp(j, "created_at");
  event.updated_at = optional_timestamp(j, "updated_at");
}

void from_json(const Json& j, CorporateActionEventsResponse& response) {
  if (j.contains("events") && j.at("events").is_array()) {
    response.events = j.at("events").get<std::vector<CorporateActionEvent>>();
  } else {
    response.events.clear();
  }
  if (j.contains("next_page_token") && !j.at("next_page_token").is_null()) {
    response.next_page_token = j.at("next_page_token").get<std::string>();
  } else {
    response.next_page_token = std::nullopt;
  }
}

}  // namespace alpaca
