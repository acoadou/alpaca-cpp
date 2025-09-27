#include "alpaca/models/Clock.hpp"

namespace alpaca {

void from_json(Json const& j, Clock& clock) {
    j.at("is_open").get_to(clock.is_open);
    j.at("next_open").get_to(clock.next_open);
    j.at("next_close").get_to(clock.next_close);
    j.at("timestamp").get_to(clock.timestamp);
}

} // namespace alpaca
