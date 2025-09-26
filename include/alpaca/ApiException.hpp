#pragma once

#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace alpaca {

/// Exception thrown when the Alpaca API returns an error response.
class ApiException : public std::runtime_error {
 public:
  ApiException(long status_code, std::string message, std::string body,
               std::unordered_map<std::string, std::string> headers)
      : std::runtime_error(std::move(message)),
        status_code_(status_code),
        body_(std::move(body)),
        headers_(std::move(headers)),
        retry_after_(parse_retry_after(headers_)) {}

  /// HTTP status code associated with the error response.
  [[nodiscard]] long status_code() const noexcept { return status_code_; }

  /// Raw response body for diagnostics.
  [[nodiscard]] const std::string& body() const noexcept { return body_; }

  /// Response headers returned with the error.
  [[nodiscard]] const std::unordered_map<std::string, std::string>& headers() const noexcept { return headers_; }

  /// Optional Retry-After hint provided by the server.
  [[nodiscard]] std::optional<std::chrono::seconds> retry_after() const noexcept { return retry_after_; }

 private:
  static bool iequals(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
          std::tolower(static_cast<unsigned char>(rhs[i]))) {
        return false;
      }
    }
    return true;
  }

  static std::time_t to_time_t_utc(std::tm tm) {
    tm.tm_isdst = 0;
#if defined(_WIN32)
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
  }

  static std::optional<std::chrono::seconds> parse_http_date(std::string_view value) {
    static constexpr const char* kHttpDateFormats[] = {
        "%a, %d %b %Y %H:%M:%S GMT",
        "%A, %d-%b-%y %H:%M:%S GMT",
        "%a %b %e %H:%M:%S %Y",
    };

    const std::string date_string{value};
    for (const char* format : kHttpDateFormats) {
      std::tm tm{};
      std::istringstream stream(date_string);
      stream.imbue(std::locale::classic());
      stream >> std::get_time(&tm, format);
      if (stream.fail()) {
        continue;
      }

      const std::time_t epoch = to_time_t_utc(tm);
      if (epoch == static_cast<std::time_t>(-1)) {
        continue;
      }

      const auto retry_time = std::chrono::system_clock::from_time_t(epoch);
      const auto now = std::chrono::system_clock::now();
      if (retry_time <= now) {
        return std::chrono::seconds::zero();
      }
      return std::chrono::duration_cast<std::chrono::seconds>(retry_time - now);
    }

    return std::nullopt;
  }

  static std::optional<std::chrono::seconds> parse_retry_after(
      const std::unordered_map<std::string, std::string>& headers) {
    for (const auto& [key, value] : headers) {
      if (iequals(key, "Retry-After")) {
        try {
          const long long seconds = std::stoll(value);
          if (seconds >= 0) {
            return std::chrono::seconds{seconds};
          }
        } catch (const std::exception&) {
          const auto parsed = parse_http_date(value);
          if (parsed.has_value()) {
            return parsed;
          }
        }
      }
    }
    return std::nullopt;
  }

  long status_code_;
  std::string body_;
  std::unordered_map<std::string, std::string> headers_;
  std::optional<std::chrono::seconds> retry_after_;
};

}  // namespace alpaca

