#include "alpaca/RestClient.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "alpaca/ApiException.hpp"
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
} // namespace

RestClient::RestClient(Configuration config, HttpClientPtr http_client, std::string base_url)
  : RestClient(std::move(config), std::move(http_client), std::move(base_url), Options{}) {}

RestClient::RestClient(Configuration config, HttpClientPtr http_client, std::string base_url, Options options)
  : config_(std::move(config)), http_client_(std::move(http_client)), base_url_(std::move(base_url)),
    options_(std::move(options)) {
    if (!http_client_) {
        throw std::invalid_argument("RestClient requires a non-null HttpClient instance");
    }
    if (!config_.has_credentials()) {
        throw std::invalid_argument("Configuration must contain API credentials");
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

        if (options_.post_request_hook) {
            options_.post_request_hook(attempt_request, response);
        }

        if (response.status_code < 400) {
            return response;
        }

        std::string message = "HTTP " + std::to_string(response.status_code);
        try {
            Json error_body = Json::parse(response.body);
            if (error_body.contains("message")) {
                message = error_body.at("message").get<std::string>();
            }
        } catch (std::exception const&) {
            // Ignore parse errors and retain the default message.
        }

        ApiException exception(response.status_code, message, response.body, response.headers);
        if (!should_retry(response.status_code, attempt)) {
            throw exception;
        }

        ++attempt;
        if (backoff.count() > 0) {
            std::this_thread::sleep_for(backoff);
            backoff = next_backoff(backoff);
        }
    }
}

std::optional<std::string> RestClient::request_raw(HttpMethod method, std::string const& path, QueryParams const& params,
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
