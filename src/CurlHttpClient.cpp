#include "alpaca/internal/CurlHttpClient.hpp"
#include "alpaca/Exceptions.hpp"
#include "alpaca/HttpClientFactory.hpp"

#include <curl/curl.h>

#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace alpaca {
namespace {
std::once_flag g_curl_init_flag;
std::once_flag g_curl_cleanup_flag;

void ensure_curl_global_init() {
    std::call_once(g_curl_init_flag, []() {
        if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
            throw CurlException(ErrorCode::CurlInitializationFailure, "Failed to initialize libcurl",
                                "curl_global_init");
        }
        std::call_once(g_curl_cleanup_flag, []() {
            std::atexit([]() {
                curl_global_cleanup();
            });
        });
    });
}

size_t write_body(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t write_header(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* headers = static_cast<HttpHeaders*>(userdata);
    std::string header_line(buffer, size * nitems);
    auto const separator = header_line.find(':');
    if (separator == std::string::npos) {
        return size * nitems;
    }

    std::string key = header_line.substr(0, separator);
    std::string value = header_line.substr(separator + 1);

    auto trim = [](std::string& text) {
        auto const start = text.find_first_not_of(" \t\r\n");
        auto const end = text.find_last_not_of(" \t\r\n");
        if (start == std::string::npos || end == std::string::npos) {
            text.clear();
        } else {
            text = text.substr(start, end - start + 1);
        }
    };

    trim(key);
    trim(value);
    if (!key.empty()) {
        headers->append(std::move(key), std::move(value));
    }
    return size * nitems;
}

class CurlSlistDeleter {
  public:
    void operator()(curl_slist* list) const {
        curl_slist_free_all(list);
    }
};

class CurlEasyDeleter {
  public:
    void operator()(CURL* handle) const {
        if (handle != nullptr) {
            curl_easy_cleanup(handle);
        }
    }
};

} // namespace

struct CurlHttpClient::Impl {
    struct HandleLease {
        HandleLease() = default;
        HandleLease(Impl* owner, std::size_t index, CURL* handle) : owner_(owner), index_(index), handle_(handle) {
        }

        HandleLease(HandleLease const&) = delete;
        HandleLease& operator=(HandleLease const&) = delete;

        HandleLease(HandleLease&& other) noexcept {
            owner_ = other.owner_;
            index_ = other.index_;
            handle_ = other.handle_;
            other.owner_ = nullptr;
            other.handle_ = nullptr;
        }

        HandleLease& operator=(HandleLease&& other) noexcept {
            if (this != &other) {
                release();
                owner_ = other.owner_;
                index_ = other.index_;
                handle_ = other.handle_;
                other.owner_ = nullptr;
                other.handle_ = nullptr;
            }
            return *this;
        }

        ~HandleLease() {
            release();
        }

        [[nodiscard]] CURL* get() const noexcept {
            return handle_;
        }

      private:
        void release() {
            if (owner_ != nullptr) {
                owner_->release_handle(index_);
                owner_ = nullptr;
                handle_ = nullptr;
            }
        }

        Impl* owner_{nullptr};
        std::size_t index_{0};
        CURL* handle_{nullptr};
    };

    explicit Impl(CurlHttpClientOptions options) : options_(std::move(options)) {
        if (options_.connection_pool_size == 0) {
            options_.connection_pool_size = 1;
        }
        ensure_curl_global_init();
        handles_.reserve(options_.connection_pool_size);
        available_indices_.reserve(options_.connection_pool_size);
        for (std::size_t i = 0; i < options_.connection_pool_size; ++i) {
            std::unique_ptr<CURL, CurlEasyDeleter> handle(curl_easy_init());
            if (!handle) {
                throw CurlException(ErrorCode::CurlHandleCreationFailure, "Failed to create CURL handle",
                                    "curl_easy_init");
            }
            handles_.push_back(std::move(handle));
            available_indices_.push_back(i);
        }
    }

    HandleLease acquire_handle() {
        std::unique_lock<std::mutex> lock(mutex_);
        available_cv_.wait(lock, [&]() {
            return !available_indices_.empty();
        });
        auto index = available_indices_.back();
        available_indices_.pop_back();
        CURL* handle = handles_[index].get();
        return HandleLease(this, index, handle);
    }

    void release_handle(std::size_t index) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            available_indices_.push_back(index);
        }
        available_cv_.notify_one();
    }

    CurlHttpClientOptions options_{};
    std::vector<std::unique_ptr<CURL, CurlEasyDeleter>> handles_{};
    std::vector<std::size_t> available_indices_{};
    std::mutex mutex_;
    std::condition_variable available_cv_;
};

CurlHttpClient::CurlHttpClient(CurlHttpClientOptions options) : impl_(new Impl(std::move(options))) {
}

CurlHttpClient::~CurlHttpClient() {
    delete impl_;
}

HttpResponse CurlHttpClient::send(HttpRequest const& request) {
    ensure_curl_global_init();

    auto lease = impl_->acquire_handle();
    CURL* handle = lease.get();
    if (!handle) {
        throw CurlException(ErrorCode::CurlHandleNotInitialized, "CURL handle is not initialized", "acquire_handle");
    }

    curl_easy_reset(handle);

    std::string response_body;
    HttpHeaders response_headers;

    curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &write_header);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, impl_->options_.follow_redirects ? 1L : 0L);
    if (impl_->options_.follow_redirects) {
        long const max_redirects = impl_->options_.max_redirects < 0 ? 0L : impl_->options_.max_redirects;
        curl_easy_setopt(handle, CURLOPT_MAXREDIRS, max_redirects);
        if (impl_->options_.restrict_redirect_protocols) {
#if defined(CURLPROTO_HTTP) && defined(CURLPROTO_HTTPS)
            curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS_STR, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif
#if defined(CURLOPT_REDIR_PROTOCOLS_STR)
            curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#endif
        }
    }

    if (request.timeout.count() > 0) {
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, static_cast<long>(request.timeout.count()));
    }

    switch (request.method) {
    case HttpMethod::GET:
        curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
        break;
    case HttpMethod::POST:
        curl_easy_setopt(handle, CURLOPT_POST, 1L);
        break;
    case HttpMethod::PUT:
        curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PUT");
        break;
    case HttpMethod::PATCH:
        curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PATCH");
        break;
    case HttpMethod::DELETE_:
        curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    }

    if (!request.body.empty()) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, request.body.size());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, request.body.c_str());
    }

    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, request.verify_peer ? 1L : 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, request.verify_host ? 2L : 0L);
    if (!request.ca_bundle_path.empty()) {
        curl_easy_setopt(handle, CURLOPT_CAINFO, request.ca_bundle_path.c_str());
    }
    if (!request.ca_bundle_dir.empty()) {
        curl_easy_setopt(handle, CURLOPT_CAPATH, request.ca_bundle_dir.c_str());
    }

    std::unique_ptr<curl_slist, CurlSlistDeleter> headers(nullptr);
    for (auto const& [key, value] : request.headers) {
        std::string header_line = key + ": " + value;
        curl_slist* raw = headers.release();
        raw = curl_slist_append(raw, header_line.c_str());
        if (!raw) {
            throw CurlException(ErrorCode::CurlHeaderAppendFailure, "Failed to append HTTP header",
                                "curl_slist_append");
        }
        headers.reset(raw);
    }
    if (headers) {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers.get());
    }

    CURLcode result = curl_easy_perform(handle);
    long status_code = 0;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status_code);
    if (result != CURLE_OK) {
        throw CurlException(ErrorCode::CurlPerformFailure,
                            std::string("curl_easy_perform failed: ") + curl_easy_strerror(result), "curl_easy_perform",
                            result);
    }

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, nullptr);

    return HttpResponse{status_code, std::move(response_body), std::move(response_headers)};
}

HttpClientPtr create_default_http_client() {
    return create_default_http_client(CurlHttpClientOptions{});
}

HttpClientPtr create_default_http_client(CurlHttpClientOptions const& options) {
    return std::make_shared<CurlHttpClient>(options);
}

HttpClientPtr ensure_http_client(HttpClientPtr& client) {
    if (!client) {
        client = create_default_http_client();
    }
    return client;
}

} // namespace alpaca
