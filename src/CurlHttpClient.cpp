#include "alpaca/internal/CurlHttpClient.hpp"
#include "alpaca/HttpClientFactory.hpp"

#include <curl/curl.h>

#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace alpaca {
namespace {
std::once_flag g_curl_init_flag;
std::once_flag g_curl_cleanup_flag;

void ensure_curl_global_init() {
    std::call_once(g_curl_init_flag, []() {
        if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
            throw std::runtime_error("Failed to initialize libcurl");
        }
        std::call_once(g_curl_cleanup_flag, []() {
            std::atexit([]() {
                curl_global_cleanup();
            });
        });
    });
}

size_t write_body(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto *body = static_cast<std::string *>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t write_header(char *buffer, size_t size, size_t nitems, void *userdata) {
    auto *headers = static_cast<std::unordered_map<std::string, std::string> *>(userdata);
    std::string header_line(buffer, size * nitems);
    auto const separator = header_line.find(':');
    if (separator != std::string::npos) {
        std::string key = header_line.substr(0, separator);
        std::string value = header_line.substr(separator + 1);
        // Trim whitespace.
        auto const start = value.find_first_not_of(" \t\r\n");
        auto const end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            value = value.substr(start, end - start + 1);
        } else {
            value.clear();
        }
        (*headers)[std::move(key)] = std::move(value);
    }
    return size * nitems;
}

class CurlSlistDeleter {
  public:
    void operator()(curl_slist *list) const {
        curl_slist_free_all(list);
    }
};

class CurlEasyDeleter {
  public:
    void operator()(CURL *handle) const {
        if (handle != nullptr) {
            curl_easy_cleanup(handle);
        }
    }
};

} // namespace

struct CurlHttpClient::Impl {
    Impl() {
        ensure_curl_global_init();
        handle.reset(curl_easy_init());
        if (!handle) {
            throw std::runtime_error("Failed to create CURL handle");
        }
    }

    std::unique_ptr<CURL, CurlEasyDeleter> handle;
    std::mutex mutex;
};

CurlHttpClient::CurlHttpClient() : impl_(new Impl()) {
}

CurlHttpClient::~CurlHttpClient() {
    delete impl_;
}

HttpResponse CurlHttpClient::send(HttpRequest const& request) {
    ensure_curl_global_init();

    std::lock_guard<std::mutex> lock(impl_->mutex);
    CURL *handle = impl_->handle.get();
    if (!handle) {
        throw std::runtime_error("CURL handle is not initialized");
    }

    curl_easy_reset(handle);

    std::string response_body;
    std::unordered_map<std::string, std::string> response_headers;

    curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &write_header);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

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
        curl_slist *raw = headers.release();
        raw = curl_slist_append(raw, header_line.c_str());
        if (!raw) {
            throw std::runtime_error("Failed to append HTTP header");
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
        throw std::runtime_error(std::string("curl_easy_perform failed: ") + curl_easy_strerror(result));
    }

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, nullptr);

    return HttpResponse{status_code, std::move(response_body), std::move(response_headers)};
}

HttpClientPtr create_default_http_client() {
    return std::make_shared<CurlHttpClient>();
}

} // namespace alpaca
