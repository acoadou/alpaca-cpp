#pragma once

#include <optional>
#include <string>

#include <chrono>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Represents a single trading day entry.
struct CalendarDay {
    std::string date;
    std::string open;
    std::string close;
};

void from_json(Json const& j, CalendarDay& day);

/// Request parameters accepted by the calendar endpoint.
struct CalendarRequest {
    std::optional<std::chrono::sys_days> start{};
    std::optional<std::chrono::sys_days> end{};

    [[nodiscard]] QueryParams to_query_params() const;
};

} // namespace alpaca
