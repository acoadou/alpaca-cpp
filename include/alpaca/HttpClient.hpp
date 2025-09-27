#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace alpaca {

/// Enumeration of HTTP methods supported by the Alpaca REST client.
enum class HttpMethod {
    GET,
    POST,
    PUT,
    PATCH,
    DELETE_
};

/// Represents an HTTP request issued by the REST client.
struct HttpRequest {
    HttpMethod method{HttpMethod::GET};
    std::string url;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::chrono::milliseconds timeout{std::chrono::milliseconds{0}};
    bool verify_peer{true};
    bool verify_host{true};
    std::string ca_bundle_path;
    std::string ca_bundle_dir;
};

/// Represents the result of an HTTP request.
struct HttpResponse {
    long status_code{0};
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

/// Defines the interface used to issue HTTP requests.
class HttpClient {
  public:
    virtual ~HttpClient() = default;

    /// Sends a request and returns the response.
    virtual HttpResponse send(HttpRequest const& request) = 0;
};

using HttpClientPtr = std::shared_ptr<HttpClient>;

} // namespace alpaca
