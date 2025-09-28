#pragma once

#include <vector>

#include "alpaca/models/Common.hpp"
#include "alpaca/models/Position.hpp"

namespace alpaca {

/// Summarizes the outcome of a bulk order cancellation attempt.
struct BulkCancelOrdersResponse {
    std::vector<CancelledOrderId> successful;
    std::vector<CancelledOrderId> failed;
};

/// Summarizes the outcome of a bulk close positions attempt.
struct BulkClosePositionsResponse {
    std::vector<ClosePositionResponse> successful;
    std::vector<ClosePositionResponse> failed;
};

} // namespace alpaca

