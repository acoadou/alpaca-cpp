#pragma once

#include <nlohmann/json.hpp>

namespace alpaca {
using Json = nlohmann::json;

template <typename Request> inline Json to_json_payload(Request const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

} // namespace alpaca
