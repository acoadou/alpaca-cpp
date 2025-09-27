#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "alpaca/ApiException.hpp"
#include "alpaca/Configuration.hpp"
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
    RestClient(Configuration config, HttpClientPtr http_client, std::string base_url);

    [[nodiscard]] Configuration const& config() const noexcept {
        return config_;
    }

    /// Performs a GET request and deserializes the JSON response into \c T.
    template <typename T> T get(std::string const& path, QueryParams const& params = {}) {
        return request_json<T>(HttpMethod::GET, path, params, std::nullopt);
    }

    /// Performs a DELETE request and deserializes the JSON response into \c T.
    template <typename T = Json> T del(std::string const& path, QueryParams const& params = {}) {
        return request_json<T>(HttpMethod::DELETE_, path, params, std::nullopt);
    }

    /// Performs a POST request with a JSON payload and returns the JSON
    /// response as \c T.
    template <typename T> T post(std::string const& path, Json const& payload, QueryParams const& params = {}) {
        return request_json<T>(HttpMethod::POST, path, params, payload.dump());
    }

    /// Performs a PUT request with a JSON payload and returns the response as
    /// \c T.
    template <typename T> T put(std::string const& path, Json const& payload, QueryParams const& params = {}) {
        return request_json<T>(HttpMethod::PUT, path, params, payload.dump());
    }

    /// Performs a PATCH request with a JSON payload and returns the response as
    /// \c T.
    template <typename T> T patch(std::string const& path, Json const& payload, QueryParams const& params = {}) {
        return request_json<T>(HttpMethod::PATCH, path, params, payload.dump());
    }

  private:
    Configuration config_;
    HttpClientPtr http_client_;
    std::string base_url_;

    static std::string build_url(std::string const& base, std::string const& path, QueryParams const& params);
    static std::string encode_query(QueryParams const& params);

    std::optional<Json> perform_request(HttpMethod method, std::string const& path, QueryParams const& params,
                                        std::optional<std::string> payload);

    template <typename T>
    T request_json(HttpMethod method, std::string const& path, QueryParams const& params,
                   std::optional<std::string> payload) {
        std::optional<Json> json = perform_request(method, path, params, std::move(payload));

        if (!json.has_value()) {
            if constexpr (std::is_void_v<T>) {
                return;
            } else if constexpr (detail::is_optional_v<T>) {
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, Json>) {
                return Json::object();
            } else if constexpr (std::is_default_constructible_v<T>) {
                return T{};
            } else {
                throw ApiException(204,
                                   "Empty response body cannot be deserialized "
                                   "into requested type",
                                   std::string{}, {});
            }
        }

        if constexpr (std::is_void_v<T>) {
            return;
        } else if constexpr (std::is_same_v<T, Json>) {
            return *json;
        } else if constexpr (detail::is_optional_v<T>) {
            using value_type = typename T::value_type;
            return T{json->get<value_type>()};
        } else {
            return json->get<T>();
        }
    }
};

} // namespace alpaca
