#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "alpaca/Configuration.hpp"
#include "alpaca/Exceptions.hpp"
#include "alpaca/HttpClient.hpp"
#include "alpaca/Json.hpp"
namespace alpaca {

/// Key-value query parameter container used by REST endpoints.
using QueryParams = std::vector<std::pair<std::string, std::string>>;

namespace detail {
template <typename T> struct is_optional : std::false_type {};

template <typename T> struct is_optional<std::optional<T>> : std::true_type {};

template <typename T> inline constexpr bool is_optional_v = is_optional<T>::value;
} // namespace detail

/// Lightweight REST client responsible for communicating with Alpaca endpoints.
class RestClient {
public:
    struct RateLimitStatus {
        std::optional<long> limit{};
        std::optional<long> remaining{};
        std::optional<long> used{};
        std::optional<std::chrono::system_clock::time_point> reset{};
    };

    struct RetryOptions {
        std::size_t max_attempts{3};
        std::chrono::milliseconds initial_backoff{std::chrono::milliseconds{100}};
        double backoff_multiplier{2.0};
        std::chrono::milliseconds max_backoff{std::chrono::seconds{5}};
        std::vector<long> retry_status_codes{429, 500, 502, 503, 504};
    };

    using PreRequestHook = std::function<void(HttpRequest&)>;
    using PostRequestHook = std::function<void(HttpRequest const&, HttpResponse const&)>;
    using AuthHandler = std::function<void(HttpRequest&, Configuration const&)>;
    using RateLimitHandler = std::function<void(RateLimitStatus const&)>;

    struct Options {
        RetryOptions retry{};
        PreRequestHook pre_request_hook{};
        PostRequestHook post_request_hook{};
        AuthHandler auth_handler{};
        RateLimitHandler rate_limit_handler{};
    };

    static RetryOptions default_retry_options();
    static Options default_options();

    RestClient(Configuration config, HttpClientPtr http_client, std::string base_url);
    RestClient(Configuration config, HttpClientPtr http_client, std::string base_url, Options options);

    [[nodiscard]] Configuration const& config() const noexcept {
        return config_;
    }

    [[nodiscard]] std::optional<RateLimitStatus> last_rate_limit_status() const;

    void set_rate_limit_handler(RateLimitHandler handler);

    /// Performs a GET request and deserializes the JSON response into \c T.
    template <typename T> T get(std::string const& path, QueryParams const& params = {}) const {
        return request_json<T>(HttpMethod::GET, path, params, std::nullopt);
    }

    /// Performs a GET request asynchronously and resolves with the JSON
    /// response deserialized into \c T.
    template <typename T> std::future<T> get_async(std::string path, QueryParams params = {}) const {
        return request_json_async<T>(HttpMethod::GET, std::move(path), std::move(params), std::nullopt);
    }

    /// Performs a DELETE request and deserializes the JSON response into \c T.
    template <typename T = Json> T del(std::string const& path, QueryParams const& params = {}) const {
        return request_json<T>(HttpMethod::DELETE_, path, params, std::nullopt);
    }

    /// Performs a DELETE request asynchronously and resolves with the JSON
    /// response deserialized into \c T.
    template <typename T = Json> std::future<T> del_async(std::string path, QueryParams params = {}) const {
        return request_json_async<T>(HttpMethod::DELETE_, std::move(path), std::move(params), std::nullopt);
    }

    /// Performs a POST request with a JSON payload and returns the JSON
    /// response as \c T.
    template <typename T> T post(std::string const& path, Json const& payload, QueryParams const& params = {}) const {
        return request_json<T>(HttpMethod::POST, path, params, payload.dump());
    }

    /// Performs a POST request asynchronously with a JSON payload and resolves
    /// with the JSON response deserialized into \c T.
    template <typename T> std::future<T> post_async(std::string path, Json payload, QueryParams params = {}) const {
        return request_json_async<T>(HttpMethod::POST, std::move(path), std::move(params), payload.dump());
    }

    /// Performs a PUT request with a JSON payload and returns the response as
    /// \c T.
    template <typename T> T put(std::string const& path, Json const& payload, QueryParams const& params = {}) const {
        return request_json<T>(HttpMethod::PUT, path, params, payload.dump());
    }

    /// Performs a PUT request asynchronously with a JSON payload and resolves
    /// with the response deserialized into \c T.
    template <typename T> std::future<T> put_async(std::string path, Json payload, QueryParams params = {}) const {
        return request_json_async<T>(HttpMethod::PUT, std::move(path), std::move(params), payload.dump());
    }

    /// Performs a PATCH request with a JSON payload and returns the response as
    /// \c T.
    template <typename T> T patch(std::string const& path, Json const& payload, QueryParams const& params = {}) const {
        return request_json<T>(HttpMethod::PATCH, path, params, payload.dump());
    }

    /// Performs a PATCH request asynchronously with a JSON payload and resolves
    /// with the response deserialized into \c T.
    template <typename T> std::future<T> patch_async(std::string path, Json payload, QueryParams params = {}) const {
        return request_json_async<T>(HttpMethod::PATCH, std::move(path), std::move(params), payload.dump());
    }

    /// Performs an HTTP request and returns the raw JSON body if present.
    [[nodiscard]] std::optional<std::string> get_raw(std::string const& path, QueryParams const& params = {}) const {
        return request_raw(HttpMethod::GET, path, params, std::nullopt);
    }

    /// Performs an HTTP GET request asynchronously and resolves with the raw
    /// JSON body when present.
    [[nodiscard]] std::future<std::optional<std::string>> get_raw_async(std::string path,
                                                                        QueryParams params = {}) const {
        return request_raw_async(HttpMethod::GET, std::move(path), std::move(params), std::nullopt);
    }

    [[nodiscard]] std::optional<std::string> del_raw(std::string const& path, QueryParams const& params = {}) const {
        return request_raw(HttpMethod::DELETE_, path, params, std::nullopt);
    }

    [[nodiscard]] std::future<std::optional<std::string>> del_raw_async(std::string path,
                                                                        QueryParams params = {}) const {
        return request_raw_async(HttpMethod::DELETE_, std::move(path), std::move(params), std::nullopt);
    }

    [[nodiscard]] std::optional<std::string> post_raw(std::string const& path, Json const& payload,
                                                      QueryParams const& params = {}) const {
        return request_raw(HttpMethod::POST, path, params, payload.dump());
    }

    [[nodiscard]] std::future<std::optional<std::string>> post_raw_async(std::string path, Json payload,
                                                                         QueryParams params = {}) const {
        return request_raw_async(HttpMethod::POST, std::move(path), std::move(params), payload.dump());
    }

    [[nodiscard]] std::optional<std::string> put_raw(std::string const& path, Json const& payload,
                                                     QueryParams const& params = {}) const {
        return request_raw(HttpMethod::PUT, path, params, payload.dump());
    }

    [[nodiscard]] std::future<std::optional<std::string>> put_raw_async(std::string path, Json payload,
                                                                        QueryParams params = {}) const {
        return request_raw_async(HttpMethod::PUT, std::move(path), std::move(params), payload.dump());
    }

    [[nodiscard]] std::optional<std::string> patch_raw(std::string const& path, Json const& payload,
                                                       QueryParams const& params = {}) const {
        return request_raw(HttpMethod::PATCH, path, params, payload.dump());
    }

    [[nodiscard]] std::future<std::optional<std::string>> patch_raw_async(std::string path, Json payload,
                                                                          QueryParams params = {}) const {
        return request_raw_async(HttpMethod::PATCH, std::move(path), std::move(params), payload.dump());
    }

private:
    Configuration config_;
    HttpClientPtr http_client_;
    std::string base_url_;
    Options options_{};
    mutable std::mutex rate_limit_mutex_;
    mutable std::optional<RateLimitStatus> last_rate_limit_status_{};

    static std::string build_url(std::string const& base, std::string const& path, QueryParams const& params);
    static std::string encode_query(QueryParams const& params);

    [[nodiscard]] std::optional<std::string> request_raw(HttpMethod method, std::string const& path,
                                                         QueryParams const& params,
                                                         std::optional<std::string> payload) const;
    HttpResponse perform_request(HttpMethod method, std::string const& path, QueryParams const& params,
                                 std::optional<std::string> payload) const;
    void apply_authentication(HttpRequest& request) const;
    [[nodiscard]] bool should_retry(long status_code, std::size_t attempt) const;
    [[nodiscard]] std::chrono::milliseconds next_backoff(std::chrono::milliseconds current) const;

    template <typename T>
    std::future<T> request_json_async(HttpMethod method, std::string path, QueryParams params,
                                      std::optional<std::string> payload) const {
        return std::async(std::launch::async, [this, method, path = std::move(path), params = std::move(params),
                          payload = std::move(payload)]() mutable {
            return this->request_json<T>(method, path, params, std::move(payload));
        });
    }

    std::future<std::optional<std::string>> request_raw_async(HttpMethod method, std::string path, QueryParams params,
                                                              std::optional<std::string> payload) const {
        return std::async(std::launch::async, [this, method, path = std::move(path), params = std::move(params),
                          payload = std::move(payload)]() mutable {
            return this->request_raw(method, path, params, std::move(payload));
        });
    }

    template <typename T>
    T request_json(HttpMethod method, std::string const& path, QueryParams const& params,
                   std::optional<std::string> payload) const {
        std::optional<std::string> body = request_raw(method, path, params, std::move(payload));

        if (!body.has_value()) {
            if constexpr (std::is_void_v<T>) {
                return;
            } else if constexpr (detail::is_optional_v<T>) {
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Json>) {
                return Json::object();
            } else if constexpr (std::is_default_constructible_v<T>) {
                return T{};
            } else {
                throw Exception(204,
                                "Empty response body cannot be deserialized "
                                "into requested type",
                                std::string{}, {});
            }
        }

        Json json = Json::parse(*body);

        if constexpr (std::is_void_v<T>) {
            return;
        } else if constexpr (std::is_same_v<T, Json>) {
            return json;
        } else if constexpr (detail::is_optional_v<T>) {
            using value_type = typename T::value_type;
            return T{json.get<value_type>()};
        } else {
            return json.get<T>();
        }
    }
};

} // namespace alpaca
