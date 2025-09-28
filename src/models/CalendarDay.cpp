#include "alpaca/models/CalendarDay.hpp"

namespace alpaca {

void from_json(Json const& j, CalendarDay& day) {
    day.date = j.value("date", std::string{});
    day.open = j.value("open", std::string{});
    day.close = j.value("close", std::string{});
}

void from_json(Json const& j, IntervalCalendar::OpenClose& interval) {
    interval.open = j.value("open", std::string{});
    interval.close = j.value("close", std::string{});
}

void from_json(Json const& j, IntervalCalendar& interval_calendar) {
    interval_calendar.date = j.value("date", std::string{});

    if (auto it = j.find("session"); it != j.end() && it->is_object()) {
        interval_calendar.session = it->get<IntervalCalendar::OpenClose>();
    } else {
        interval_calendar.session = IntervalCalendar::OpenClose{};
    }

    if (auto it = j.find("trading"); it != j.end() && it->is_object()) {
        interval_calendar.trading = it->get<IntervalCalendar::OpenClose>();
    } else {
        interval_calendar.trading = IntervalCalendar::OpenClose{};
    }
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

} // namespace alpaca
