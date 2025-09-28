#pragma once

#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "alpaca/HttpHeaders.hpp"

namespace alpaca {

/// Exception thrown when the Alpaca API returns an error response.
class ApiException : public std::runtime_error {
  public:
    ApiException(long status_code, std::string message, std::string body, HttpHeaders headers)
      : std::runtime_error(std::move(message)), status_code_(status_code), body_(std::move(body)),
        headers_(std::move(headers)), retry_after_(parse_retry_after(headers_)) {
    }

    /// HTTP status code associated with the error response.
    [[nodiscard]] long status_code() const noexcept {
        return status_code_;
    }

    /// Raw response body for diagnostics.
    [[nodiscard]] std::string const& body() const noexcept {
        return body_;
    }

    /// Response headers returned with the error.
    [[nodiscard]] HttpHeaders const& headers() const noexcept {
        return headers_;
    }

    /// Optional Retry-After hint provided by the server.
    [[nodiscard]] std::optional<std::chrono::seconds> retry_after() const noexcept {
        return retry_after_;
    }

  private:
    static bool iequals(std::string_view lhs, std::string_view rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i]))) {
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
        static constexpr char const *kHttpDateFormats[] = {
            "%a, %d %b %Y %H:%M:%S GMT",
            "%A, %d-%b-%y %H:%M:%S GMT",
            "%a %b %e %H:%M:%S %Y",
        };

        std::string const date_string{value};
        for (char const *format : kHttpDateFormats) {
            std::tm tm{};
            std::istringstream stream(date_string);
            stream.imbue(std::locale::classic());
            stream >> std::get_time(&tm, format);
            if (stream.fail()) {
                continue;
            }

            std::time_t const epoch = to_time_t_utc(tm);
            if (epoch == static_cast<std::time_t>(-1)) {
                continue;
            }

            auto const retry_time = std::chrono::system_clock::from_time_t(epoch);
            auto const now = std::chrono::system_clock::now();
            if (retry_time <= now) {
                return std::chrono::seconds::zero();
            }
            return std::chrono::duration_cast<std::chrono::seconds>(retry_time - now);
        }

        return std::nullopt;
    }

    static std::optional<std::chrono::seconds> parse_retry_after(HttpHeaders const& headers) {
        for (auto const& [key, value] : headers) {
            if (iequals(key, "Retry-After")) {
                try {
                    long long const seconds = std::stoll(value);
                    if (seconds >= 0) {
                        return std::chrono::seconds{seconds};
                    }
                } catch (std::exception const&) {
                    auto const parsed = parse_http_date(value);
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
    HttpHeaders headers_;
    std::optional<std::chrono::seconds> retry_after_;
};

/// Raised when the API rejects the provided credentials or authentication data.
class AuthenticationException : public ApiException {
  public:
    using ApiException::ApiException;
};

/// Raised when the request is understood but fails validation.
class ValidationException : public ApiException {
  public:
    using ApiException::ApiException;
};

/// Raised when the API refuses the request due to missing permissions.
class PermissionException : public ApiException {
  public:
    using ApiException::ApiException;
};

/// Raised when the requested resource cannot be located.
class NotFoundException : public ApiException {
  public:
    using ApiException::ApiException;
};

/// Raised when the API signals that the caller should slow down.
class RateLimitException : public ApiException {
  public:
    using ApiException::ApiException;
};

/// Raised for other 4xx responses.
class ClientException : public ApiException {
  public:
    using ApiException::ApiException;
};

/// Raised when the API encounters an internal error.
class ServerException : public ApiException {
  public:
    using ApiException::ApiException;
};

} // namespace alpaca
