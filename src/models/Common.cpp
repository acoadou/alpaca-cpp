#include "alpaca/models/Common.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "alpaca/Exceptions.hpp"
namespace alpaca {
namespace {
std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return value;
}
bool has_time_portion(std::string_view value) {
    return value.find('T') != std::string_view::npos || value.find('t') != std::string_view::npos ||
           value.find(' ') != std::string_view::npos;
}

std::tm make_utc_tm(std::time_t time) {
    std::tm result{};
#if defined(_WIN32)
    gmtime_s(&result, &time);
#else
    gmtime_r(&time, &result);
#endif
    return result;
}

template <typename Formatter> std::string format_with_stream(Formatter&& formatter) {
    // TODO: Replace with std::format once C++20 support is available.
    std::ostringstream oss;
    formatter(oss);
    return oss.str();
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_tm_with_stream(std::tm const& tm, char const* pattern) {
    return format_with_stream([&](std::ostringstream& oss) {
        oss << std::put_time(&tm, pattern);
    });
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_fractional_microseconds(std::chrono::microseconds fractional) {
    if (fractional.count() == 0) {
        return {};
    }

    auto formatted = format_with_stream([&](std::ostringstream& oss) {
        oss << std::setw(6) << std::setfill('0') << fractional.count();
    });

    while (!formatted.empty() && formatted.back() == '0') {
        formatted.pop_back();
    }
    return formatted;
}

[[noreturn]] void throw_timestamp_error(char const* message, std::string_view stage) {
    throw InvalidArgumentException("timestamp", message, ErrorCode::InvalidArgument,
                                   {
                                       {"stage", std::string(stage)}
    });
}

int parse_number(std::string_view value, std::size_t& pos, std::size_t count, char const* error_message) {
    if (pos + count > value.size()) {
        throw_timestamp_error(error_message, "parse_number");
    }
    int result = 0;
    for (std::size_t i = 0; i < count; ++i) {
        char const ch = value[pos + i];
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            throw_timestamp_error(error_message, "parse_number");
        }
        result = result * 10 + (ch - '0');
    }
    pos += count;
    return result;
}

void expect_char(std::string_view value, std::size_t& pos, char expected, char const* error_message) {
    if (pos >= value.size() || value[pos] != expected) {
        throw_timestamp_error(error_message, "expect_char");
    }
    ++pos;
}

std::chrono::sys_days parse_date(std::string_view value, std::size_t& pos) {
    int const year = parse_number(value, pos, 4, "Invalid year in timestamp");
    expect_char(value, pos, '-', "Expected '-' after year in timestamp");
    int const month = parse_number(value, pos, 2, "Invalid month in timestamp");
    expect_char(value, pos, '-', "Expected '-' after month in timestamp");
    int const day = parse_number(value, pos, 2, "Invalid day in timestamp");
    std::chrono::year_month_day ymd{std::chrono::year{year} / std::chrono::month{static_cast<unsigned>(month)} /
                                    std::chrono::day{static_cast<unsigned>(day)}};
    if (!ymd.ok()) {
        throw_timestamp_error("Invalid calendar date in timestamp", "parse_date");
    }
    return std::chrono::sys_days{ymd};
}

std::chrono::nanoseconds parse_fraction(std::string_view value, std::size_t& pos) {
    if (pos >= value.size() || value[pos] != '.') {
        return std::chrono::nanoseconds{0};
    }
    ++pos;
    std::int64_t nanos = 0;
    std::size_t digits = 0;
    while (pos < value.size() && std::isdigit(static_cast<unsigned char>(value[pos]))) {
        if (digits < 9) {
            nanos = nanos * 10 + (value[pos] - '0');
            ++digits;
        }
        ++pos;
    }
    while (digits < 9) {
        nanos *= 10;
        ++digits;
    }
    return std::chrono::nanoseconds{nanos};
}

std::chrono::seconds parse_timezone(std::string_view value, std::size_t& pos) {
    if (pos >= value.size()) {
        throw_timestamp_error("Missing timezone specifier in timestamp", "parse_timezone");
    }
    char const indicator = value[pos++];
    if (indicator == 'Z' || indicator == 'z') {
        return std::chrono::seconds{0};
    }
    if (indicator != '+' && indicator != '-') {
        throw_timestamp_error("Invalid timezone specifier in timestamp", "parse_timezone");
    }
    int const sign = indicator == '+' ? 1 : -1;
    int const hours = parse_number(value, pos, 2, "Invalid timezone hour in timestamp");
    if (pos < value.size() && value[pos] == ':') {
        ++pos;
    }
    int const minutes = parse_number(value, pos, 2, "Invalid timezone minute in timestamp");
    return sign * (std::chrono::hours{hours} + std::chrono::minutes{minutes});
}
} // namespace

std::string to_string(OrderSide side) {
    return side == OrderSide::BUY ? "buy" : "sell";
}

std::string to_string(OrderType type) {
    switch (type) {
    case OrderType::MARKET:
        return "market";
    case OrderType::LIMIT:
        return "limit";
    case OrderType::STOP:
        return "stop";
    case OrderType::STOP_LIMIT:
        return "stop_limit";
    case OrderType::TRAILING_STOP:
        return "trailing_stop";
    }
    throw InvalidArgumentException("order_type", "Unknown OrderType", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                           },
                                       {"value",   std::to_string(static_cast<int>(type))}
    });
}

std::string to_string(TimeInForce tif) {
    switch (tif) {
    case TimeInForce::DAY:
        return "day";
    case TimeInForce::GTC:
        return "gtc";
    case TimeInForce::OPG:
        return "opg";
    case TimeInForce::IOC:
        return "ioc";
    case TimeInForce::FOK:
        return "fok";
    case TimeInForce::GTD:
        return "gtd";
    }
    throw InvalidArgumentException("time_in_force", "Unknown TimeInForce", ErrorCode::InvalidArgument,
                                   {
                                       {"context", "to_string"                          },
                                       {"value",   std::to_string(static_cast<int>(tif))}
    });
}

std::string to_string(OrderClass order_class) {
    switch (order_class) {
    case OrderClass::SIMPLE:
        return "simple";
    case OrderClass::BRACKET:
        return "bracket";
    case OrderClass::ONE_CANCELS_OTHER:
        return "oco";
    case OrderClass::ONE_TRIGGERS_OTHER:
        return "oto";
    }
    throw InvalidArgumentException(
    "order_class", "Unknown OrderClass", ErrorCode::InvalidArgument,
    {
        {"context", "to_string"                                  },
        {"value",   std::to_string(static_cast<int>(order_class))}
    });
}

std::string to_string(AssetStatus status) {
    return status == AssetStatus::ACTIVE ? "active" : "inactive";
}

std::string to_string(AssetClass asset_class) {
    switch (asset_class) {
    case AssetClass::US_EQUITY:
        return "us_equity";
    case AssetClass::CRYPTO:
        return "crypto";
    case AssetClass::FOREX:
        return "forex";
    case AssetClass::FUTURES:
        return "futures";
    case AssetClass::OPTION:
        return "option";
    }
    throw InvalidArgumentException(
    "asset_class", "Unknown AssetClass", ErrorCode::InvalidArgument,
    {
        {"context", "to_string"                                  },
        {"value",   std::to_string(static_cast<int>(asset_class))}
    });
}

std::string to_string(SortDirection direction) {
    switch (direction) {
    case SortDirection::ASC:
        return "asc";
    case SortDirection::DESC:
        return "desc";
    }
    throw InvalidArgumentException(
    "sort_direction", "Unknown SortDirection", ErrorCode::InvalidArgument,
    {
        {"context", "to_string"                                },
        {"value",   std::to_string(static_cast<int>(direction))}
    });
}

OrderSide order_side_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "buy") {
        return OrderSide::BUY;
    }
    if (lower == "sell") {
        return OrderSide::SELL;
    }
    throw InvalidArgumentException("order_side", "Unknown order side: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                   },
                                       {"context", "order_side_from_string"}
    });
}

OrderType order_type_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "market") {
        return OrderType::MARKET;
    }
    if (lower == "limit") {
        return OrderType::LIMIT;
    }
    if (lower == "stop") {
        return OrderType::STOP;
    }
    if (lower == "stop_limit") {
        return OrderType::STOP_LIMIT;
    }
    if (lower == "trailing_stop") {
        return OrderType::TRAILING_STOP;
    }
    throw InvalidArgumentException("order_type", "Unknown order type: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                   },
                                       {"context", "order_type_from_string"}
    });
}

TimeInForce time_in_force_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "day") {
        return TimeInForce::DAY;
    }
    if (lower == "gtc") {
        return TimeInForce::GTC;
    }
    if (lower == "opg") {
        return TimeInForce::OPG;
    }
    if (lower == "ioc") {
        return TimeInForce::IOC;
    }
    if (lower == "fok") {
        return TimeInForce::FOK;
    }
    if (lower == "gtd") {
        return TimeInForce::GTD;
    }
    throw InvalidArgumentException("time_in_force", "Unknown time in force: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                      },
                                       {"context", "time_in_force_from_string"}
    });
}

OrderClass order_class_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "simple") {
        return OrderClass::SIMPLE;
    }
    if (lower == "bracket") {
        return OrderClass::BRACKET;
    }
    if (lower == "oco") {
        return OrderClass::ONE_CANCELS_OTHER;
    }
    if (lower == "oto") {
        return OrderClass::ONE_TRIGGERS_OTHER;
    }
    throw InvalidArgumentException("order_class", "Unknown order class: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                    },
                                       {"context", "order_class_from_string"}
    });
}

AssetStatus asset_status_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "active") {
        return AssetStatus::ACTIVE;
    }
    if (lower == "inactive") {
        return AssetStatus::INACTIVE;
    }
    throw InvalidArgumentException("asset_status", "Unknown asset status: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                     },
                                       {"context", "asset_status_from_string"}
    });
}

AssetClass asset_class_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "us_equity") {
        return AssetClass::US_EQUITY;
    }
    if (lower == "crypto") {
        return AssetClass::CRYPTO;
    }
    if (lower == "forex") {
        return AssetClass::FOREX;
    }
    if (lower == "futures") {
        return AssetClass::FUTURES;
    }
    if (lower == "option") {
        return AssetClass::OPTION;
    }
    throw InvalidArgumentException("asset_class", "Unknown asset class: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                    },
                                       {"context", "asset_class_from_string"}
    });
}

SortDirection sort_direction_from_string(std::string const& value) {
    std::string lower = to_lower(value);
    if (lower == "asc") {
        return SortDirection::ASC;
    }
    if (lower == "desc") {
        return SortDirection::DESC;
    }
    throw InvalidArgumentException("sort_direction", "Unknown sort direction: " + value, ErrorCode::InvalidArgument,
                                   {
                                       {"value",   value                       },
                                       {"context", "sort_direction_from_string"}
    });
}

void to_json(Json& j, PageToken const& token) {
    j = Json{
        {"next", token.next},
        {"prev", token.prev}
    };
}

void from_json(Json const& j, PageToken& token) {
    j.at("next").get_to(token.next);
    j.at("prev").get_to(token.prev);
}

void to_json(Json& j, TakeProfitParams const& params) {
    j = Json{
        {"limit_price", params.limit_price}
    };
}

void to_json(Json& j, StopLossParams const& params) {
    j = Json{};
    if (params.stop_price.has_value()) {
        j["stop_price"] = *params.stop_price;
    }
    if (params.limit_price.has_value()) {
        j["limit_price"] = *params.limit_price;
    }
}

void from_json(Json const& j, CancelledOrderId& id) {
    j.at("id").get_to(id.id);
    id.status = order_status_from_string(j.at("status").get<std::string>());
}

Timestamp parse_timestamp(std::string_view value) {
    if (value.empty()) {
        throw_timestamp_error("Unable to parse timestamp: empty", "parse_timestamp");
    }

    std::size_t pos = 0;
    auto const date = parse_date(value, pos);
    if (!has_time_portion(value)) {
        if (pos != value.size()) {
            throw_timestamp_error("Unexpected trailing characters in timestamp", "parse_timestamp_date_only");
        }
        return Timestamp{std::chrono::duration_cast<Timestamp::duration>(date.time_since_epoch())};
    }

    if (pos >= value.size() || (value[pos] != 'T' && value[pos] != 't' && value[pos] != ' ')) {
        throw_timestamp_error("Expected 'T' separator in timestamp", "parse_timestamp_separator");
    }
    ++pos;
    int const hour = parse_number(value, pos, 2, "Invalid hour in timestamp");
    expect_char(value, pos, ':', "Expected ':' after hours in timestamp");
    int const minute = parse_number(value, pos, 2, "Invalid minute in timestamp");
    expect_char(value, pos, ':', "Expected ':' after minutes in timestamp");
    int const second = parse_number(value, pos, 2, "Invalid second in timestamp");
    auto const fractional = parse_fraction(value, pos);
    auto const offset = parse_timezone(value, pos);
    if (pos != value.size()) {
        throw_timestamp_error("Unexpected trailing characters in timestamp", "parse_timestamp_trailing");
    }

    auto time_of_day =
    std::chrono::hours{hour} + std::chrono::minutes{minute} + std::chrono::seconds{second} + fractional;
    Timestamp timestamp{std::chrono::duration_cast<Timestamp::duration>(date.time_since_epoch())};
    timestamp += time_of_day;
    timestamp -= offset;
    return timestamp;
}

std::optional<Timestamp> parse_timestamp_field(Json const& j, char const* key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    auto const value = j.at(key).get<std::string>();
    if (value.empty()) {
        return std::nullopt;
    }
    return parse_timestamp(value);
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_timestamp(Timestamp timestamp) {
    auto const truncated = std::chrono::floor<std::chrono::microseconds>(timestamp);
    auto const seconds = std::chrono::floor<std::chrono::seconds>(truncated);
    auto const fractional = std::chrono::duration_cast<std::chrono::microseconds>(truncated - seconds);

    auto const time = std::chrono::system_clock::to_time_t(seconds);
    std::tm utc_tm = make_utc_tm(time);

    auto result = format_tm_with_stream(utc_tm, "%FT%T");
    auto const fraction = format_fractional_microseconds(fractional);
    if (!fraction.empty()) {
        result.push_back('.');
        result.append(fraction);
    }

    result.push_back('Z');
    return result;
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_calendar_date(std::chrono::sys_days day) {
    std::chrono::year_month_day const ymd{day};
    int const year = static_cast<int>(ymd.year());
    unsigned const month = static_cast<unsigned>(ymd.month());
    unsigned const day_of_month = static_cast<unsigned>(ymd.day());

    return format_with_stream([&](std::ostringstream& oss) {
        oss << year << '-' << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << std::setfill('0')
            << day_of_month;
    });
}

std::string join_csv(std::vector<std::string> const& values) {
    if (values.empty()) {
        return {};
    }
    std::size_t total = values.size() - 1;
    for (auto const& value : values) {
        total += value.size();
    }
    std::string result;
    result.reserve(total);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0U) {
            result.push_back(',');
        }
        result.append(values[i]);
    }
    return result;
}

} // namespace alpaca
