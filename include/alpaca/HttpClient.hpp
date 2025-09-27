#pragma once

#include <chrono>
#include <future>
#include <memory>
#include <string>

#include "alpaca/HttpHeaders.hpp"
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
    HttpHeaders headers;
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
    HttpHeaders headers;
};

/// Defines the interface used to issue HTTP requests.
class HttpClient {
  public:
    virtual ~HttpClient() = default;

    /// Sends a request and returns the response.
    virtual HttpResponse send(HttpRequest const& request) = 0;

    /// Sends a request asynchronously and resolves with the response once the
    /// operation completes. Implementations may override this to integrate with
    /// custom executors or event loops; the default simply delegates to
    /// `send()` on a background task.
    virtual std::future<HttpResponse> send_async(HttpRequest request) {
        return std::async(std::launch::async, [this, request = std::move(request)]() mutable {
            return this->send(request);
        });
    }
};

using HttpClientPtr = std::shared_ptr<HttpClient>;

} // namespace alpaca
