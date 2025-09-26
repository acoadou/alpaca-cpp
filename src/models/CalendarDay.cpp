#include "alpaca/models/CalendarDay.hpp"

namespace alpaca {

void from_json(const Json& j, CalendarDay& day) {
  day.date = j.value("date", std::string{});
  day.open = j.value("open", std::string{});
  day.close = j.value("close", std::string{});
}

QueryParams CalendarRequest::to_query_params() const {
  QueryParams params;
  if (start.has_value()) {
    params.emplace_back("start", format_calendar_date(*start));
  }
  if (end.has_value()) {
    params.emplace_back("end", format_calendar_date(*end));
  }
  return params;
}

}  // namespace alpaca
