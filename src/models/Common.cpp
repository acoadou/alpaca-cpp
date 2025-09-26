#include "alpaca/models/Common.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace alpaca {
namespace {
std::string to_lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
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

template <typename Formatter>
std::string format_with_stream(Formatter&& formatter) {
  // TODO: Replace with std::format once C++20 support is available.
  std::ostringstream oss;
  formatter(oss);
  return oss.str();
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_tm_with_stream(const std::tm& tm, const char* pattern) {
  return format_with_stream([&](std::ostringstream& oss) { oss << std::put_time(&tm, pattern); });
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

int parse_number(std::string_view value, std::size_t& pos, std::size_t count, const char* error_message) {
  if (pos + count > value.size()) {
    throw std::invalid_argument(error_message);
  }
  int result = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const char ch = value[pos + i];
    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      throw std::invalid_argument(error_message);
    }
    result = result * 10 + (ch - '0');
  }
  pos += count;
  return result;
}

void expect_char(std::string_view value, std::size_t& pos, char expected, const char* error_message) {
  if (pos >= value.size() || value[pos] != expected) {
    throw std::invalid_argument(error_message);
  }
  ++pos;
}

std::chrono::sys_days parse_date(std::string_view value, std::size_t& pos) {
  const int year = parse_number(value, pos, 4, "Invalid year in timestamp");
  expect_char(value, pos, '-', "Expected '-' after year in timestamp");
  const int month = parse_number(value, pos, 2, "Invalid month in timestamp");
  expect_char(value, pos, '-', "Expected '-' after month in timestamp");
  const int day = parse_number(value, pos, 2, "Invalid day in timestamp");
  std::chrono::year_month_day ymd{std::chrono::year{year} / std::chrono::month{static_cast<unsigned>(month)} /
                                 std::chrono::day{static_cast<unsigned>(day)}};
  if (!ymd.ok()) {
    throw std::invalid_argument("Invalid calendar date in timestamp");
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
    throw std::invalid_argument("Missing timezone specifier in timestamp");
  }
  const char indicator = value[pos++];
  if (indicator == 'Z' || indicator == 'z') {
    return std::chrono::seconds{0};
  }
  if (indicator != '+' && indicator != '-') {
    throw std::invalid_argument("Invalid timezone specifier in timestamp");
  }
  const int sign = indicator == '+' ? 1 : -1;
  const int hours = parse_number(value, pos, 2, "Invalid timezone hour in timestamp");
  if (pos < value.size() && value[pos] == ':') {
    ++pos;
  }
  const int minutes = parse_number(value, pos, 2, "Invalid timezone minute in timestamp");
  return sign * (std::chrono::hours{hours} + std::chrono::minutes{minutes});
}
}  // namespace

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
  throw std::invalid_argument("Unknown OrderType");
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
  throw std::invalid_argument("Unknown TimeInForce");
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
  throw std::invalid_argument("Unknown OrderClass");
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
  }
  throw std::invalid_argument("Unknown AssetClass");
}

std::string to_string(SortDirection direction) {
  switch (direction) {
    case SortDirection::ASC:
      return "asc";
    case SortDirection::DESC:
      return "desc";
  }
  throw std::invalid_argument("Unknown SortDirection");
}

OrderSide order_side_from_string(const std::string& value) {
  std::string lower = to_lower(value);
  if (lower == "buy") {
    return OrderSide::BUY;
  }
  if (lower == "sell") {
    return OrderSide::SELL;
  }
  throw std::invalid_argument("Unknown order side: " + value);
}

OrderType order_type_from_string(const std::string& value) {
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
  throw std::invalid_argument("Unknown order type: " + value);
}

TimeInForce time_in_force_from_string(const std::string& value) {
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
  throw std::invalid_argument("Unknown time in force: " + value);
}

OrderClass order_class_from_string(const std::string& value) {
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
  throw std::invalid_argument("Unknown order class: " + value);
}

AssetStatus asset_status_from_string(const std::string& value) {
  std::string lower = to_lower(value);
  if (lower == "active") {
    return AssetStatus::ACTIVE;
  }
  if (lower == "inactive") {
    return AssetStatus::INACTIVE;
  }
  throw std::invalid_argument("Unknown asset status: " + value);
}

AssetClass asset_class_from_string(const std::string& value) {
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
  throw std::invalid_argument("Unknown asset class: " + value);
}

SortDirection sort_direction_from_string(const std::string& value) {
  std::string lower = to_lower(value);
  if (lower == "asc") {
    return SortDirection::ASC;
  }
  if (lower == "desc") {
    return SortDirection::DESC;
  }
  throw std::invalid_argument("Unknown sort direction: " + value);
}

void to_json(Json& j, const PageToken& token) {
  j = Json{{"next", token.next}, {"prev", token.prev}};
}

void from_json(const Json& j, PageToken& token) {
  j.at("next").get_to(token.next);
  j.at("prev").get_to(token.prev);
}

void to_json(Json& j, const TakeProfitParams& params) {
  j = Json{{"limit_price", params.limit_price}};
}

void to_json(Json& j, const StopLossParams& params) {
  j = Json{};
  if (params.stop_price.has_value()) {
    j["stop_price"] = *params.stop_price;
  }
  if (params.limit_price.has_value()) {
    j["limit_price"] = *params.limit_price;
  }
}

void from_json(const Json& j, CancelledOrderId& id) {
  j.at("id").get_to(id.id);
  j.at("status").get_to(id.status);
}

Timestamp parse_timestamp(std::string_view value) {
  if (value.empty()) {
    throw std::invalid_argument("Unable to parse timestamp: empty");
  }

  std::size_t pos = 0;
  const auto date = parse_date(value, pos);
  if (!has_time_portion(value)) {
    if (pos != value.size()) {
      throw std::invalid_argument("Unexpected trailing characters in timestamp");
    }
    return Timestamp{std::chrono::duration_cast<Timestamp::duration>(date.time_since_epoch())};
  }

  if (pos >= value.size() || (value[pos] != 'T' && value[pos] != 't' && value[pos] != ' ')) {
    throw std::invalid_argument("Expected 'T' separator in timestamp");
  }
  ++pos;
  const int hour = parse_number(value, pos, 2, "Invalid hour in timestamp");
  expect_char(value, pos, ':', "Expected ':' after hours in timestamp");
  const int minute = parse_number(value, pos, 2, "Invalid minute in timestamp");
  expect_char(value, pos, ':', "Expected ':' after minutes in timestamp");
  const int second = parse_number(value, pos, 2, "Invalid second in timestamp");
  const auto fractional = parse_fraction(value, pos);
  const auto offset = parse_timezone(value, pos);
  if (pos != value.size()) {
    throw std::invalid_argument("Unexpected trailing characters in timestamp");
  }

  auto time_of_day = std::chrono::hours{hour} + std::chrono::minutes{minute} + std::chrono::seconds{second} + fractional;
  Timestamp timestamp{std::chrono::duration_cast<Timestamp::duration>(date.time_since_epoch())};
  timestamp += time_of_day;
  timestamp -= offset;
  return timestamp;
}

std::optional<Timestamp> parse_timestamp_field(const Json& j, const char* key) {
  if (!j.contains(key) || j.at(key).is_null()) {
    return std::nullopt;
  }
  const auto value = j.at(key).get<std::string>();
  if (value.empty()) {
    return std::nullopt;
  }
  return parse_timestamp(value);
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_timestamp(Timestamp timestamp) {
  const auto truncated = std::chrono::floor<std::chrono::microseconds>(timestamp);
  const auto seconds = std::chrono::floor<std::chrono::seconds>(truncated);
  const auto fractional = std::chrono::duration_cast<std::chrono::microseconds>(truncated - seconds);

  const auto time = std::chrono::system_clock::to_time_t(seconds);
  std::tm utc_tm = make_utc_tm(time);

  auto result = format_tm_with_stream(utc_tm, "%FT%T");
  const auto fraction = format_fractional_microseconds(fractional);
  if (!fraction.empty()) {
    result.push_back('.');
    result.append(fraction);
  }

  result.push_back('Z');
  return result;
}

// TODO: Replace with std::format once C++20 support is available.
std::string format_calendar_date(std::chrono::sys_days day) {
  const std::chrono::year_month_day ymd{day};
  const int year = static_cast<int>(ymd.year());
  const unsigned month = static_cast<unsigned>(ymd.month());
  const unsigned day_of_month = static_cast<unsigned>(ymd.day());

  return format_with_stream([&](std::ostringstream& oss) {
    oss << year << '-' << std::setw(2) << std::setfill('0') << month << '-' << std::setw(2) << std::setfill('0')
        << day_of_month;
  });
}

std::string join_csv(const std::vector<std::string>& values) {
  if (values.empty()) {
    return {};
  }
  std::size_t total = values.size() - 1;
  for (const auto& value : values) {
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

}  // namespace alpaca

