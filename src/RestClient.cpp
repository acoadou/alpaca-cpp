#include "alpaca/RestClient.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <locale>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <ctime>

#include "alpaca/Exceptions.hpp"
#include "alpaca/Exceptions.hpp"
#include "alpaca/version.hpp"

namespace alpaca {
namespace {
bool is_unreserved(unsigned char c) {
    return std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.' || c == '~';
}

std::string url_encode(std::string_view value) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex << std::uppercase;
    for (unsigned char c : value) {
        if (is_unreserved(c)) {
            encoded << static_cast<char>(c);
        } else {
            encoded << '%' << std::setw(2) << static_cast<int>(c);
        }
    }
    return encoded.str();
}

std::optional<long> parse_long_header(HttpHeaders const& headers, std::string_view key) {
    auto it = headers.find(key);
    if (it == headers.end()) {
        return std::nullopt;
    }
    try {
        return std::stol(it->second);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::chrono::system_clock::time_point> parse_reset_header(HttpHeaders const& headers,
                                                                        std::string_view key) {
    auto raw = parse_long_header(headers, key);
    if (!raw.has_value()) {
        return std::nullopt;
    }
    return std::chrono::system_clock::time_point{std::chrono::seconds{*raw}};
}

std::optional<RestClient::RateLimitStatus> extract_rate_limit(HttpHeaders const& headers) {
    RestClient::RateLimitStatus status;
    status.limit = parse_long_header(headers, "x-ratelimit-limit");
    status.remaining = parse_long_header(headers, "x-ratelimit-remaining");
    status.used = parse_long_header(headers, "x-ratelimit-used");
    status.reset = parse_reset_header(headers, "x-ratelimit-reset");

    if (!status.limit && !status.remaining && !status.used && !status.reset) {
        return std::nullopt;
    }
    return status;
}

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

bool code_matches(std::optional<std::string> const& code, std::initializer_list<std::string_view> expected) {
    if (!code.has_value()) {
        return false;
    }
    auto lowered_code = to_lower_copy(*code);
    for (auto candidate : expected) {
        if (lowered_code == to_lower_copy(candidate)) {
            return true;
        }
    }
    return false;
}

bool equals_ignore_case(std::string_view lhs, std::string_view rhs) {
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
        if (!equals_ignore_case(key, "Retry-After")) {
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

bool is_idempotent(HttpMethod method) {
    switch (method) {
    case HttpMethod::GET:
    case HttpMethod::PUT:
    case HttpMethod::DELETE_:
        return true;
    case HttpMethod::POST:
    case HttpMethod::PATCH:
        return false;
    }
    return false;
}

[[noreturn]] void throw_for_api_error(long status_code, std::string message, std::string body, HttpHeaders headers,
                                      std::optional<std::string> const& error_code) {
    std::string lowered_message = to_lower_copy(message);

    if (status_code == 401 || code_matches(error_code, {"40110000", "authentication_error", "unauthorized"}) ||
        message_contains(lowered_message, {"authentication", "credential", "unauthorized"})) {
        throw AuthenticationException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 403 || code_matches(error_code, {"forbidden", "permission_denied", "insufficient_permission"}) ||
        message_contains(lowered_message, {"forbidden", "permission"})) {
        throw PermissionException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 404 || code_matches(error_code, {"40410000", "not_found", "resource_not_found"}) ||
        message_contains(lowered_message, {"not found"})) {
        throw NotFoundException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 429 ||
        code_matches(error_code, {"42910000", "rate_limit", "too_many_requests", "rate_limit_exceeded"}) ||
        message_contains(lowered_message, {"rate limit", "too many request", "throttle"})) {
        throw RateLimitException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code >= 500 || code_matches(error_code, {"50010000", "internal_error", "service_unavailable"}) ||
        message_contains(lowered_message, {"internal server", "service unavailable", "server error"})) {
        throw ServerException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code == 422 || status_code == 400 || code_matches(error_code, {"validation_error", "invalid_request"}) ||
        message_contains(lowered_message, {"validation", "invalid"})) {
        throw ValidationException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    if (status_code >= 400 && status_code < 500) {
        throw ClientException(status_code, std::move(message), std::move(body), std::move(headers));
    }

    throw Exception(status_code, std::move(message), std::move(body), std::move(headers));
}
} // namespace

RestClient::RetryOptions RestClient::default_retry_options() {
    return RetryOptions{};
}

RestClient::Options RestClient::default_options() {
    Options options;
    options.retry = default_retry_options();
    return options;
}

RestClient::RestClient(Configuration config, HttpClientPtr http_client, std::string base_url)
    : RestClient(std::move(config), std::move(http_client), std::move(base_url), default_options()) {}

RestClient::RestClient(Configuration config, HttpClientPtr http_client, std::string base_url, Options options)
    : config_(std::move(config)), http_client_(std::move(http_client)), base_url_(std::move(base_url)),
    options_(std::move(options)) {
    if (!http_client_) {
        throw InvalidArgumentException("http_client", "RestClient requires a non-null HttpClient instance",
                                       ErrorCode::HttpClientRequired);
    }
    if (!config_.has_credentials()) {
        throw InvalidArgumentException("credentials", "Configuration must contain API credentials",
                                       ErrorCode::RestClientConfigurationMissing);
    }
    if (options_.retry.max_attempts < 1) {
        options_.retry.max_attempts = 1;
    }
    if (options_.retry.initial_backoff.count() < 0) {
        options_.retry.initial_backoff = std::chrono::milliseconds{0};
    }
    if (options_.retry.max_backoff.count() < 0) {
        options_.retry.max_backoff = std::chrono::milliseconds{0};
    }
    if (options_.retry.max_jitter.count() < 0) {
        options_.retry.max_jitter = std::chrono::milliseconds{0};
    }
    if (options_.retry.retry_after_max.count() < 0) {
        options_.retry.retry_after_max = std::chrono::milliseconds{0};
    }
}

std::string RestClient::build_url(std::string const& base, std::string const& path, QueryParams const& params) {
    std::ostringstream url;
    url << base;
    if (!path.empty() && path.front() != '/') {
        url << '/';
    }
    url << path;
    std::string const query = encode_query(params);
    if (!query.empty()) {
        url << '?' << query;
    }
    return url.str();
}

std::string RestClient::encode_query(QueryParams const& params) {
    if (params.empty()) {
        return {};
    }
    std::ostringstream query;
    bool first = true;
    for (auto const& [key, value] : params) {
        if (!first) {
            query << '&';
        }
        first = false;
        query << url_encode(key) << '=' << url_encode(value);
    }
    return query.str();
}

HttpResponse RestClient::perform_request(HttpMethod method, std::string const& path, QueryParams const& params,
                                         std::optional<std::string> payload) const {
    std::string url = build_url(base_url_, path, params);

    HttpRequest request;
    request.method = method;
    request.url = std::move(url);
    request.timeout = config_.timeout;
    request.headers = config_.default_headers;

    if (payload.has_value()) {
        request.body = std::move(*payload);
        request.headers["Content-Type"] = "application/json";
    }

    std::size_t attempt = 0;
    std::chrono::milliseconds backoff = options_.retry.initial_backoff;

    while (true) {
        HttpRequest attempt_request = request;
        apply_authentication(attempt_request);
        if (options_.pre_request_hook) {
            options_.pre_request_hook(attempt_request);
        }

        HttpResponse response;
        try {
            response = http_client_->send(attempt_request);
        } catch (std::exception const&) {
            if (!should_retry(method, std::nullopt, attempt)) {
                throw;
            }

            ++attempt;
            auto const delay = compute_retry_delay(std::nullopt, backoff);
            if (delay.count() > 0) {
                std::this_thread::sleep_for(delay);
            }
            backoff = next_backoff(backoff);
            continue;
        }

        auto rate_limit_status = extract_rate_limit(response.headers);
        {
            std::lock_guard<std::mutex> lock(rate_limit_mutex_);
            last_rate_limit_status_ = rate_limit_status;
        }
        if (rate_limit_status.has_value() && options_.rate_limit_handler) {
            options_.rate_limit_handler(*rate_limit_status);
        }

        if (options_.post_request_hook) {
            options_.post_request_hook(attempt_request, response);
        }

        if (response.status_code < 400) {
            return response;
        }

        std::string message = "HTTP " + std::to_string(response.status_code);
        std::optional<std::string> error_code;
        try {
            Json error_body = Json::parse(response.body);
            if (error_body.contains("message") && error_body.at("message").is_string()) {
                message = error_body.at("message").get<std::string>();
            }
            if (error_body.contains("code")) {
                auto const& node = error_body.at("code");
                if (node.is_string()) {
                    error_code = node.get<std::string>();
                } else if (node.is_number_integer()) {
                    error_code = std::to_string(node.get<long long>());
                }
            }
        } catch (std::exception const&) {
            // Ignore parse errors and retain the default message.
        }

        if (!should_retry(method, response.status_code, attempt)) {
            ThrowException(response.status_code, std::move(message), std::move(response.body),
                           std::move(response.headers), error_code);
        }

        ++attempt;
        auto const retry_after = parse_retry_after(response.headers);
        auto const delay = compute_retry_delay(retry_after, backoff);
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }
        backoff = next_backoff(backoff);
    }
}

std::optional<RestClient::RateLimitStatus> RestClient::last_rate_limit_status() const {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    return last_rate_limit_status_;
}

void RestClient::set_rate_limit_handler(RateLimitHandler handler) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    options_.rate_limit_handler = std::move(handler);
}

std::optional<std::string> RestClient::request_raw(HttpMethod method, std::string const& path,
                                                   QueryParams const& params,
                                                   std::optional<std::string> payload) const {
    HttpResponse response = perform_request(method, path, params, std::move(payload));

    if (response.body.empty()) {
        return std::nullopt;
    }

    return response.body;
}

void RestClient::apply_authentication(HttpRequest& request) const {
    if (options_.auth_handler) {
        options_.auth_handler(request, config_);
    } else {
        bool const has_key_secret = !config_.api_key_id.empty() && !config_.api_secret_key.empty();
        if (has_key_secret) {
            request.headers["APCA-API-KEY-ID"] = config_.api_key_id;
            request.headers["APCA-API-SECRET-KEY"] = config_.api_secret_key;
        } else {
            bool const has_authorization_header = request.headers.find("Authorization") != request.headers.end();
            if (!has_authorization_header && config_.bearer_token.has_value() && !config_.bearer_token->empty()) {
                request.headers["Authorization"] = std::string("Bearer ") + *config_.bearer_token;
            }
        }
    }

    if (request.headers.find("Accept") == request.headers.end()) {
        request.headers["Accept"] = "application/json";
    }
    if (request.headers.find("User-Agent") == request.headers.end()) {
        request.headers["User-Agent"] = std::string("alpaca-cpp/") + std::string(kVersion);
    }
    request.verify_peer = config_.verify_ssl;
    request.verify_host = config_.verify_hostname;
    request.ca_bundle_path = config_.ca_bundle_path;
    request.ca_bundle_dir = config_.ca_bundle_dir;
}

bool RestClient::should_retry(HttpMethod method, std::optional<long> status_code, std::size_t attempt) const {
    if (attempt + 1 >= options_.retry.max_attempts) {
        return false;
    }

    if (options_.retry.retry_classifier) {
        return options_.retry.retry_classifier(method, status_code, attempt);
    }

    if (!is_idempotent(method)) {
        return false;
    }

    if (!status_code.has_value()) {
        return true;
    }

    auto const& retryable_codes = options_.retry.retry_status_codes;
    return std::find(retryable_codes.begin(), retryable_codes.end(), *status_code) != retryable_codes.end();
}

std::chrono::milliseconds RestClient::next_backoff(std::chrono::milliseconds current) const {
    if (current.count() <= 0) {
        return current;
    }

    double next = static_cast<double>(current.count()) * options_.retry.backoff_multiplier;
    auto next_duration = std::chrono::milliseconds{static_cast<long long>(next)};
    if (options_.retry.max_backoff.count() > 0 && next_duration > options_.retry.max_backoff) {
        next_duration = options_.retry.max_backoff;
    }
    return next_duration;
}

std::chrono::milliseconds RestClient::compute_retry_delay(std::optional<std::chrono::seconds> retry_after,
                                                          std::chrono::milliseconds backoff) const {
    if (retry_after.has_value()) {
        if (options_.retry.retry_after_max.count() == 0) {
            return std::chrono::milliseconds{0};
        }
        auto capped = std::chrono::duration_cast<std::chrono::milliseconds>(*retry_after);
        if (options_.retry.retry_after_max.count() > 0 && capped > options_.retry.retry_after_max) {
            capped = options_.retry.retry_after_max;
        }
        return capped;
    }

    return apply_jitter(backoff);
}

std::chrono::milliseconds RestClient::apply_jitter(std::chrono::milliseconds base) const {
    if (options_.retry.max_jitter.count() <= 0) {
        return base;
    }

    static thread_local std::mt19937 generator{std::random_device{}()};
    std::uniform_int_distribution<long long> distribution(0, options_.retry.max_jitter.count());
    auto const jitter = std::chrono::milliseconds{distribution(generator)};
    return base + jitter;
}

} // namespace alpaca
