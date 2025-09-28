#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace alpaca {

class HttpHeaders;

enum class ErrorCode {
    Unknown,
    HeaderNotFound,
    CurlInitializationFailure,
    CurlHandleCreationFailure,
    CurlHandleNotInitialized,
    CurlHeaderAppendFailure,
    CurlPerformFailure,
    WebSocketSendQueueLimit,
    InvalidPingInterval,
    NullBackfillCoordinator,
    InvalidArgument,
    OAuthConfigurationError,
    MarketDataConfigurationError,
    RestClientConfigurationMissing,
    HttpClientRequired,
    ApiResponseError,
};

class Exception : public std::runtime_error {
  public:
    using Metadata = std::unordered_map<std::string, std::string>;

    Exception(ErrorCode code, std::string message, Metadata metadata = {});

    Exception(long status_code, std::string message, std::string body, HttpHeaders const& headers);

    Exception(ErrorCode code, std::string message, Metadata metadata, long status_code, std::string body,
              HttpHeaders const& headers);

    [[nodiscard]] ErrorCode code() const noexcept {
        return code_;
    }

    [[nodiscard]] Metadata const& metadata() const noexcept {
        return metadata_;
    }

    [[nodiscard]] bool has_http_context() const noexcept {
        return status_code_.has_value();
    }

    [[nodiscard]] long status_code() const noexcept {
        return status_code_.value_or(0);
    }

    [[nodiscard]] std::optional<long> status_code_opt() const noexcept {
        return status_code_;
    }

    [[nodiscard]] std::string const& body() const noexcept {
        return body_;
    }

    [[nodiscard]] HttpHeaders const& headers() const;

    [[nodiscard]] std::optional<std::chrono::seconds> retry_after() const noexcept {
        return retry_after_;
    }

  protected:
    void set_http_context(long status_code, std::string body, HttpHeaders const& headers);

  private:
    ErrorCode code_;
    Metadata metadata_;
    std::optional<long> status_code_{};
    std::string body_{};
    struct HeadersDeleter {
        void operator()(HttpHeaders const *ptr) const noexcept;
    };
    std::unique_ptr<HttpHeaders const, HeadersDeleter> headers_{};
    std::optional<std::chrono::seconds> retry_after_{};
};

/// Raised when the API rejects the provided credentials or authentication data.
class AuthenticationException : public Exception {
  public:
    using Exception::Exception;
};

/// Raised when the request is understood but fails validation.
class ValidationException : public Exception {
  public:
    using Exception::Exception;
};

/// Raised when the API refuses the request due to missing permissions.
class PermissionException : public Exception {
  public:
    using Exception::Exception;
};

/// Raised when the requested resource cannot be located.
class NotFoundException : public Exception {
  public:
    using Exception::Exception;
};

/// Raised when the API signals that the caller should slow down.
class RateLimitException : public Exception {
  public:
    using Exception::Exception;
};

/// Raised for other 4xx responses.
class ClientException : public Exception {
  public:
    using Exception::Exception;
};

/// Raised when the API encounters an internal error.
class ServerException : public Exception {
  public:
    using Exception::Exception;
};

class HeaderNotFoundException : public Exception {
  public:
    explicit HeaderNotFoundException(std::string header_name)
      : Exception(ErrorCode::HeaderNotFound, "header not found",
                  {
                      {"header", header_name}
    }),
        header_name_(std::move(header_name)) {
    }

    [[nodiscard]] std::string const& header_name() const noexcept {
        return header_name_;
    }

  private:
    std::string header_name_;
};

class CurlException : public Exception {
  public:
    CurlException(ErrorCode code, std::string message, std::string operation,
                  std::optional<long> native_error = std::nullopt)
      : Exception(code, std::move(message), create_metadata(operation, native_error)), operation_(std::move(operation)),
        native_error_(native_error) {
    }

    [[nodiscard]] std::string const& operation() const noexcept {
        return operation_;
    }

    [[nodiscard]] std::optional<long> native_error() const noexcept {
        return native_error_;
    }

  private:
    static Metadata create_metadata(std::string const& operation, std::optional<long> native_error) {
        Metadata metadata{
            {"operation", operation}
        };
        if (native_error.has_value()) {
            metadata.emplace("native_error", std::to_string(*native_error));
        }
        return metadata;
    }

    std::string operation_;
    std::optional<long> native_error_;
};

class InvalidArgumentException : public Exception {
  public:
    InvalidArgumentException(std::string argument_name, std::string message,
                             ErrorCode code = ErrorCode::InvalidArgument, Metadata metadata = {})
      : Exception(code, std::move(message), enrich_metadata(std::move(metadata), argument_name)),
        argument_name_(std::move(argument_name)) {
    }

    [[nodiscard]] std::string const& argument_name() const noexcept {
        return argument_name_;
    }

  private:
    static Metadata enrich_metadata(Metadata metadata, std::string const& argument_name) {
        metadata.emplace("argument", argument_name);
        return metadata;
    }

    std::string argument_name_;
};

class StreamingException : public Exception {
  public:
    StreamingException(ErrorCode code, std::string message, Metadata metadata = {})
      : Exception(code, std::move(message), std::move(metadata)) {
    }
};

class WebSocketQueueLimitException : public StreamingException {
  public:
    WebSocketQueueLimitException(std::size_t limit)
      : StreamingException(ErrorCode::WebSocketSendQueueLimit, "websocket send queue limit reached",
                           {
                               {"limit", std::to_string(limit)}
    }),
        limit_(limit) {
    }

    [[nodiscard]] std::size_t limit() const noexcept {
        return limit_;
    }

  private:
    std::size_t limit_;
};

/// Classifies an API error response and throws the appropriate Exception subtype.
[[noreturn]] void ThrowException(long status_code, std::string message, std::string body, HttpHeaders headers,
                                 std::optional<std::string> error_code = std::nullopt);

} // namespace alpaca
