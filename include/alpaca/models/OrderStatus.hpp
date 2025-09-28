#pragma once

#include <string>

namespace alpaca {

/// Enumerates the lifecycle states an order can report via the Alpaca API.
enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    DONE_FOR_DAY,
    CANCELED,
    EXPIRED,
    REPLACED,
    PENDING_CANCEL,
    PENDING_REPLACE,
    ACCEPTED,
    PENDING_NEW,
    ACCEPTED_FOR_BIDDING,
    STOPPED,
    REJECTED,
    SUSPENDED,
    CALCULATED,
    HELD,
    UNKNOWN
};

/// Converts an order status enum to the string representation expected by the API.
std::string to_string(OrderStatus status);

/// Parses an API order status string into the strongly typed enum.
OrderStatus order_status_from_string(std::string const& value);

} // namespace alpaca

