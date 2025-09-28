#include "alpaca/RestClient.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

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

        HttpResponse response = http_client_->send(attempt_request);

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

        if (!should_retry(response.status_code, attempt)) {
            ThrowException(response.status_code, std::move(message), std::move(response.body),
                           std::move(response.headers), error_code);
        }

        ++attempt;
        if (backoff.count() > 0) {
            std::this_thread::sleep_for(backoff);
            backoff = next_backoff(backoff);
        }
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

bool RestClient::should_retry(long status_code, std::size_t attempt) const {
    if (attempt + 1 >= options_.retry.max_attempts) {
        return false;
    }

    return std::find(options_.retry.retry_status_codes.begin(), options_.retry.retry_status_codes.end(), status_code) !=
        options_.retry.retry_status_codes.end();
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

} // namespace alpaca
