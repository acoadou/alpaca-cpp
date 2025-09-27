#pragma once

#include <chrono>

#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Returns the current UTC timestamp using the SDK's Timestamp alias.
[[nodiscard]] inline Timestamp utc_now() {
    return std::chrono::time_point_cast<Timestamp::duration>(std::chrono::system_clock::now());
}

/// Convenience helper producing a timestamp \p duration ago from now.
template <typename Rep, typename Period>
[[nodiscard]] inline Timestamp since(std::chrono::duration<Rep, Period> duration) {
    return utc_now() - std::chrono::duration_cast<Timestamp::duration>(duration);
}

/// Convenience helper producing a timestamp \p duration in the future from now.
template <typename Rep, typename Period>
[[nodiscard]] inline Timestamp until(std::chrono::duration<Rep, Period> duration) {
    return utc_now() + std::chrono::duration_cast<Timestamp::duration>(duration);
}

} // namespace alpaca
