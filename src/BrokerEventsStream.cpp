#include "alpaca/Streaming.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <curl/curl.h>
#include <iomanip>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "alpaca/Exceptions.hpp"
#include "alpaca/version.hpp"

namespace alpaca::streaming {
namespace {

std::string url_encode(std::string_view value) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex << std::uppercase;
    for (unsigned char c : value) {
        if (std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << static_cast<char>(c);
        } else {
            encoded << '%' << std::setw(2) << static_cast<int>(c);
        }
    }
    return encoded.str();
}

std::string encode_query(std::vector<std::pair<std::string, std::string>> const& params) {
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

std::pair<std::size_t, std::size_t> find_event_delimiter(std::string const& buffer) {
    auto const pos_lf = buffer.find("\n\n");
    auto const pos_crlf = buffer.find("\r\n\r\n");
    if (pos_lf == std::string::npos && pos_crlf == std::string::npos) {
        return {std::string::npos, 0};
    }
    if (pos_lf == std::string::npos) {
        return {pos_crlf, 4};
    }
    if (pos_crlf == std::string::npos) {
        return {pos_lf, 2};
    }
    if (pos_crlf < pos_lf) {
        return {pos_crlf, 4};
    }
    return {pos_lf, 2};
}

class CurlBrokerEventsTransport : public detail::BrokerEventsTransport {
  public:
    explicit CurlBrokerEventsTransport(Parameters params) : params_(std::move(params)) {
    }

    void run(DataCallback on_data, CloseCallback on_close) override {
        stop_requested_.store(false);
        aborted_by_stop_.store(false);
        callback_exception_ = nullptr;
        on_data_ = &on_data;
        on_close_ = &on_close;

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), &curl_easy_cleanup);
        if (!handle) {
            throw std::runtime_error("curl_easy_init failed");
        }

        struct curl_slist* header_list = nullptr;
        auto header_guard = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>{nullptr, &curl_slist_free_all};
        try {
            for (auto const& [name, value] : params_.headers) {
                std::string header = name + ": " + value;
                header_list = curl_slist_append(header_list, header.c_str());
                if (!header_list) {
                    throw std::runtime_error("curl_slist_append failed");
                }
            }
            header_guard.reset(header_list);

            curl_easy_setopt(handle.get(), CURLOPT_URL, params_.url.c_str());
            curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, header_list);
            curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, &CurlBrokerEventsTransport::write_callback);
            curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, this);
            curl_easy_setopt(handle.get(), CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(handle.get(), CURLOPT_XFERINFOFUNCTION, &CurlBrokerEventsTransport::progress_callback);
            curl_easy_setopt(handle.get(), CURLOPT_XFERINFODATA, this);
            curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, params_.verify_peer ? 1L : 0L);
            curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, params_.verify_host ? 2L : 0L);
            if (!params_.ca_bundle_path.empty()) {
                curl_easy_setopt(handle.get(), CURLOPT_CAINFO, params_.ca_bundle_path.c_str());
            }
            if (!params_.ca_bundle_dir.empty()) {
                curl_easy_setopt(handle.get(), CURLOPT_CAPATH, params_.ca_bundle_dir.c_str());
            }
            curl_easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L);
            if (params_.timeout.count() > 0) {
                curl_easy_setopt(handle.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(params_.timeout.count()));
            }

            auto const result = curl_easy_perform(handle.get());
            if (callback_exception_) {
                std::rethrow_exception(callback_exception_);
            }

            if (aborted_by_stop_.load() && (result == CURLE_ABORTED_BY_CALLBACK || result == CURLE_OK)) {
                return;
            }

            if (result != CURLE_OK && result != CURLE_WRITE_ERROR) {
                throw std::runtime_error(std::string{"curl_easy_perform failed: "} + curl_easy_strerror(result));
            }

            if (!stop_requested_.load()) {
                (*on_close_)();
            }
        } catch (...) {
            throw;
        }
    }

    void stop() override {
        stop_requested_.store(true);
    }

  private:
    static std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
        auto* self = static_cast<CurlBrokerEventsTransport*>(userdata);
        auto total = size * nmemb;
        if (total == 0) {
            return 0;
        }
        if (self->stop_requested_.load()) {
            self->aborted_by_stop_.store(true);
            return 0;
        }
        try {
            (*self->on_data_)(std::string_view(ptr, total));
        } catch (...) {
            self->callback_exception_ = std::current_exception();
            return 0;
        }
        return total;
    }

    static int progress_callback(void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
        auto* self = static_cast<CurlBrokerEventsTransport*>(clientp);
        if (self->stop_requested_.load()) {
            self->aborted_by_stop_.store(true);
            return 1;
        }
        return 0;
    }

    Parameters params_{};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> aborted_by_stop_{false};
    detail::BrokerEventsTransport::DataCallback* on_data_{nullptr};
    detail::BrokerEventsTransport::CloseCallback* on_close_{nullptr};
    std::exception_ptr callback_exception_{};
};

} // namespace

BrokerEventsStream::BrokerEventsStream(Configuration config, BrokerEventsStreamOptions options)
  : config_(std::move(config)), options_(std::move(options)), last_event_id_(options_.last_event_id),
    transport_factory_(nullptr), rng_(std::random_device{}()) {
    if (options_.request_timeout.count() < 0) {
        options_.request_timeout = std::chrono::milliseconds{0};
    }
    if (options_.reconnect.initial_delay.count() < 0) {
        options_.reconnect.initial_delay = std::chrono::milliseconds{0};
    }
    if (options_.reconnect.max_delay.count() < 0) {
        options_.reconnect.max_delay = std::chrono::milliseconds{0};
    }
    if (options_.reconnect.jitter.count() < 0) {
        options_.reconnect.jitter = std::chrono::milliseconds{0};
    }
}

BrokerEventsStream::~BrokerEventsStream() {
    stop();
}

void BrokerEventsStream::start() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (running_) {
        throw std::runtime_error("BrokerEventsStream already running");
    }
    stop_requested_.store(false);
    running_.store(true);
    worker_ = std::thread([this]() {
        this->run();
    });
}

void BrokerEventsStream::stop() {
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!running_) {
        return;
    }
    stop_requested_.store(true);
    detail::BrokerEventsTransport* transport = nullptr;
    {
        std::lock_guard<std::mutex> transport_lock(transport_mutex_);
        transport = active_transport_;
    }
    lock.unlock();
    if (transport != nullptr) {
        transport->stop();
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    lock.lock();
    running_.store(false);
    stop_requested_.store(false);
}

bool BrokerEventsStream::is_running() const noexcept {
    return running_.load();
}

void BrokerEventsStream::set_event_handler(EventHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    event_handler_ = std::move(handler);
}

void BrokerEventsStream::set_error_handler(ErrorHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    error_handler_ = std::move(handler);
}

void BrokerEventsStream::set_transport_factory_for_testing(TransportFactory factory) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (running_) {
        throw std::runtime_error("transport factory cannot be changed while running");
    }
    transport_factory_ = std::move(factory);
}

std::optional<std::string> BrokerEventsStream::last_event_id() const {
    std::lock_guard<std::mutex> lock(last_event_mutex_);
    return last_event_id_;
}

void BrokerEventsStream::run() {
    std::size_t consecutive_failures = 0;
    while (!stop_requested_.load()) {
        std::unique_ptr<detail::BrokerEventsTransport> transport;
        try {
            transport = make_transport();
        } catch (...) {
            dispatch_error(std::current_exception());
            ++consecutive_failures;
            if (stop_requested_.load()) {
                break;
            }
            auto delay = compute_backoff_delay(consecutive_failures);
            if (delay.count() > 0) {
                std::this_thread::sleep_for(delay);
            }
            continue;
        }
        if (!transport) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(transport_mutex_);
            active_transport_ = transport.get();
        }

        bool dispatched_in_session = false;
        bool had_exception = false;
        std::string buffer;
        try {
            transport->run(
            [this, &buffer, &dispatched_in_session](std::string_view chunk) {
                if (stop_requested_.load()) {
                    return;
                }
                if (handle_data_chunk(buffer, chunk)) {
                    dispatched_in_session = true;
                }
            },
            []() {
            });
        } catch (...) {
            dispatch_error(std::current_exception());
            had_exception = true;
        }

        {
            std::lock_guard<std::mutex> lock(transport_mutex_);
            active_transport_ = nullptr;
        }

        if (stop_requested_.load()) {
            break;
        }

        if (dispatched_in_session) {
            consecutive_failures = 0;
        } else {
            ++consecutive_failures;
        }

        auto delay =
        consecutive_failures == 0 ? std::chrono::milliseconds{0} : compute_backoff_delay(consecutive_failures);
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }
    }

    running_.store(false);
}

bool BrokerEventsStream::handle_data_chunk(std::string& buffer, std::string_view chunk) {
    buffer.append(chunk.data(), chunk.size());
    bool dispatched = false;
    while (true) {
        auto const [pos, length] = find_event_delimiter(buffer);
        if (pos == std::string::npos) {
            break;
        }
        std::string block = buffer.substr(0, pos);
        buffer.erase(0, pos + length);
        if (block.empty()) {
            continue;
        }
        if (process_event_block(block)) {
            dispatched = true;
        }
    }
    return dispatched;
}

bool BrokerEventsStream::process_event_block(std::string const& block) {
    std::istringstream stream(block);
    std::string line;
    std::optional<std::string> event_id;
    std::string data_payload;
    bool has_data = false;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (line.front() == ':') {
            continue;
        }
        auto const colon = line.find(':');
        std::string field = colon == std::string::npos ? line : line.substr(0, colon);
        std::string value;
        if (colon != std::string::npos) {
            value = line.substr(colon + 1);
            if (!value.empty() && value.front() == ' ') {
                value.erase(value.begin());
            }
        }
        if (field == "id") {
            event_id = value;
        } else if (field == "data") {
            if (has_data) {
                data_payload.push_back('\n');
            }
            data_payload += value;
            has_data = true;
        }
    }

    if (event_id.has_value()) {
        std::lock_guard<std::mutex> lock(last_event_mutex_);
        last_event_id_ = *event_id;
    }

    if (!has_data) {
        return false;
    }

    return process_event_data(data_payload);
}

bool BrokerEventsStream::process_event_data(std::string const& data) {
    try {
        Json payload = Json::parse(data);
        if (payload.is_array()) {
            bool dispatched = false;
            for (auto const& entry : payload) {
                BrokerEvent event = entry.get<BrokerEvent>();
                dispatch_event(std::move(event));
                dispatched = true;
            }
            return dispatched;
        }
        if (payload.is_object()) {
            BrokerEvent event = payload.get<BrokerEvent>();
            dispatch_event(std::move(event));
            return true;
        }
    } catch (...) {
        dispatch_error(std::current_exception());
    }
    return false;
}

void BrokerEventsStream::dispatch_event(BrokerEvent event) {
    EventHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = event_handler_;
    }
    if (handler) {
        try {
            handler(event);
        } catch (...) {
            dispatch_error(std::current_exception());
        }
    }
}

void BrokerEventsStream::dispatch_error(std::exception_ptr error) {
    if (!error) {
        return;
    }
    ErrorHandler handler;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handler = error_handler_;
    }
    if (handler) {
        handler(std::move(error));
    }
}

std::string BrokerEventsStream::build_url() const {
    std::ostringstream url;
    url << config_.broker_base_url;

    std::string path = options_.resource;
    if (path.empty()) {
        path = "accounts";
    }

    auto normalized = path;
    if (normalized.rfind("/v2/", 0) == std::string::npos && normalized.rfind("v2/", 0) == std::string::npos) {
        normalized = "v2/events/" + normalized;
    }

    if (normalized.front() != '/') {
        url << '/';
    }
    url << normalized;

    std::string const query = encode_query(options_.query);
    if (!query.empty()) {
        url << '?' << query;
    }
    return url.str();
}

HttpHeaders BrokerEventsStream::build_headers() const {
    HttpHeaders headers = config_.default_headers;
    for (auto const& entry : options_.headers) {
        headers.append(entry.first, entry.second);
    }

    bool const has_key_secret = !config_.api_key_id.empty() && !config_.api_secret_key.empty();
    if (has_key_secret) {
        headers.set("APCA-API-KEY-ID", config_.api_key_id);
        headers.set("APCA-API-SECRET-KEY", config_.api_secret_key);
    } else {
        if (headers.find("Authorization") == headers.end()) {
            if (config_.bearer_token.has_value() && !config_.bearer_token->empty()) {
                headers.set("Authorization", std::string{"Bearer "}.append(*config_.bearer_token));
            }
        }
    }

    if (headers.find("Accept") == headers.end()) {
        headers.set("Accept", "text/event-stream");
    }
    if (headers.find("User-Agent") == headers.end()) {
        headers.set("User-Agent", std::string{"alpaca-cpp/"} + std::string(kVersion));
    }
    if (headers.find("Last-Event-ID") == headers.end()) {
        std::lock_guard<std::mutex> lock(last_event_mutex_);
        if (last_event_id_.has_value()) {
            headers.set("Last-Event-ID", *last_event_id_);
        }
    }

    headers.set("Cache-Control", "no-cache");
    headers.set("Connection", "keep-alive");

    return headers;
}

std::unique_ptr<detail::BrokerEventsTransport> BrokerEventsStream::make_transport() const {
    detail::BrokerEventsTransport::Parameters params;
    params.url = build_url();
    params.headers = build_headers();
    params.timeout = options_.request_timeout;
    params.verify_peer = config_.verify_ssl;
    params.verify_host = config_.verify_hostname;
    params.ca_bundle_path = config_.ca_bundle_path;
    params.ca_bundle_dir = config_.ca_bundle_dir;

    if (transport_factory_) {
        return transport_factory_(params);
    }
    return std::make_unique<CurlBrokerEventsTransport>(std::move(params));
}

std::chrono::milliseconds BrokerEventsStream::compute_backoff_delay(std::size_t attempt) {
    if (attempt == 0) {
        attempt = 1;
    }
    double factor = std::pow(options_.reconnect.multiplier, static_cast<double>(attempt - 1));
    auto base_delay = static_cast<long long>(static_cast<double>(options_.reconnect.initial_delay.count()) * factor);
    if (base_delay <= 0) {
        base_delay = options_.reconnect.initial_delay.count();
    }
    std::chrono::milliseconds delay{base_delay};
    if (delay > options_.reconnect.max_delay) {
        delay = options_.reconnect.max_delay;
    }

    if (options_.reconnect.jitter.count() > 0) {
        std::uniform_int_distribution<long long> dist(0, options_.reconnect.jitter.count());
        auto jitter = std::chrono::milliseconds(dist(rng_));
        if (delay + jitter > options_.reconnect.max_delay) {
            delay = options_.reconnect.max_delay;
        } else {
            delay += jitter;
        }
    }

    if (delay.count() <= 0) {
        delay = options_.reconnect.initial_delay;
    }
    return delay;
}

} // namespace alpaca::streaming
