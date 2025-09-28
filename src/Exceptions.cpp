#include "alpaca/Exceptions.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <exception>
#include <initializer_list>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "alpaca/HttpHeaders.hpp"

namespace alpaca {
namespace {

std::string to_lower_copy(std::string_view value) {
    std::string lowered(value);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered;
}

bool message_contains(std::string const& lowered_message, std::initializer_list<std::string_view> fragments) {
    for (auto fragment : fragments) {
        if (lowered_message.find(to_lower_copy(fragment)) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool code_matches(std::optional<std::string> const& lowered_code, std::initializer_list<std::string_view> expected) {
    if (!lowered_code.has_value()) {
        return false;
    }
    for (auto candidate : expected) {
        if (*lowered_code == to_lower_copy(candidate)) {
            return true;
        }
    }
    return false;
}

bool iequals(std::string_view lhs, std::string_view rhs) {
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

std::time_t to_time_t_utc(std::tm tm) {
    tm.tm_isdst = 0;
#if defined(_WIN32)
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

std::optional<std::chrono::seconds> parse_http_date(std::string_view value) {
    static constexpr char const* kHttpDateFormats[] = {
        "%a, %d %b %Y %H:%M:%S GMT",
        "%A, %d-%b-%y %H:%M:%S GMT",
        "%a %b %e %H:%M:%S %Y",
    };

    std::string const date_string{value};
    for (char const* format : kHttpDateFormats) {
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

std::optional<std::chrono::seconds> parse_retry_after(HttpHeaders const& headers) {
    for (auto const& [key, value] : headers) {
        if (!iequals(key, "Retry-After")) {
            continue;
        }

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
    return std::nullopt;
}

} // namespace

void Exception::HeadersDeleter::operator()(HttpHeaders const* ptr) const noexcept {
    delete ptr;
}

Exception::Exception(ErrorCode code, std::string message, Metadata metadata)
    : std::runtime_error(std::move(message)), code_(code), metadata_(std::move(metadata)) {}

Exception::Exception(ErrorCode code, std::string message, Metadata metadata, long status_code, std::string body,
                     HttpHeaders const& headers)
    : Exception(code, std::move(message), std::move(metadata)) {
    set_http_context(status_code, std::move(body), headers);
}

Exception::Exception(long status_code, std::string message, std::string body, HttpHeaders const& headers)
    : Exception(ErrorCode::ApiResponseError, std::move(message), Metadata{}) {
    set_http_context(status_code, std::move(body), headers);
}

void Exception::set_http_context(long status_code, std::string body, HttpHeaders const& headers) {
    status_code_ = status_code;
    body_ = std::move(body);
    headers_.reset(new HttpHeaders(headers));
    retry_after_ = parse_retry_after(*headers_);

    metadata_["status_code"] = std::to_string(status_code);
    metadata_["body_length"] = std::to_string(body_.size());
    metadata_["header_count"] = std::to_string(headers_->size());
}

HttpHeaders const& Exception::headers() const {
    if (headers_) {
        return *headers_;
    }
    static HttpHeaders const empty{};
    return empty;
}

[[noreturn]] void ThrowException(long status_code, std::string message, std::string body, HttpHeaders headers,
                                 std::optional<std::string> error_code) {
    std::optional<std::string> lowered_code;
    if (error_code.has_value()) {
        lowered_code = to_lower_copy(*error_code);
    }

    std::string lowered_message = to_lower_copy(message);

    if (status_code == 401 ||
        code_matches(lowered_code, {"40110000", "authentication_error", "unauthorized", "invalid_client",
                                    "invalid_grant", "authentication_failed", "client_authentication_failed"}) ||
        message_contains(lowered_message, {"authentication", "credential", "unauthorized"})) {
        throw AuthenticationException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 403 ||
        code_matches(lowered_code, {"forbidden", "permission_denied", "insufficient_permission", "access_denied",
                                    "unauthorized_client"}) ||
        message_contains(lowered_message, {"forbidden", "permission", "access denied"})) {
        throw PermissionException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 404 || code_matches(lowered_code, {"40410000", "not_found", "resource_not_found"}) ||
        message_contains(lowered_message, {"not found"})) {
        throw NotFoundException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 429 ||
        code_matches(lowered_code,
                     {"42910000", "rate_limit", "too_many_requests", "rate_limit_exceeded", "slow_down"}) ||
        message_contains(lowered_message, {"rate limit", "too many request", "throttle", "slow down"})) {
        throw RateLimitException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code >= 500 || code_matches(lowered_code, {"50010000", "internal_error", "service_unavailable"}) ||
        message_contains(lowered_message, {"internal server", "service unavailable", "server error"})) {
        throw ServerException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 422 || status_code == 400 ||
        code_matches(lowered_code,
                     {"validation_error", "invalid_request", "invalid_scope", "unsupported_response_type"}) ||
        message_contains(lowered_message, {"validation", "invalid", "unsupported response", "invalid scope"})) {
        throw ValidationException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code >= 400 && status_code < 500) {
        throw ClientException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    throw Exception(status_code, std::move(message), std::move(body), std::move(headers));
}

} // namespace alpaca
