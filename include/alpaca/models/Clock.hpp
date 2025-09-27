#pragma once

#include <string>

#include "alpaca/Json.hpp"

namespace alpaca {

/// Represents the trading clock returned by the Alpaca API.
struct Clock {
    bool is_open{false};
    std::string next_open;
    std::string next_close;
    std::string timestamp;
};

void from_json(Json const& j, Clock& clock);

} // namespace alpaca
