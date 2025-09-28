#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <optional>

#include "alpaca/HttpClient.hpp"

namespace {

class CapturingHttpClient : public alpaca::HttpClient {
  public:
    alpaca::HttpResponse send(alpaca::HttpRequest const& request) override {
        std::lock_guard<std::mutex> lock(mutex_);
        last_request_ = request;
        ++call_count_;
        alpaca::HttpResponse response;
        response.status_code = 201;
        response.body = "created";
        return response;
    }

    std::optional<alpaca::HttpRequest> last_request() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_request_;
    }

    int call_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return call_count_;
    }

  private:
    mutable std::mutex mutex_;
    std::optional<alpaca::HttpRequest> last_request_{};
    int call_count_{0};
};

TEST(HttpClientTest, SendAsyncDelegatesToSend) {
    CapturingHttpClient client;
    alpaca::HttpRequest request;
    request.method = alpaca::HttpMethod::POST;
    request.url = "https://example.com";
    request.body = "payload";
    request.timeout = std::chrono::milliseconds{1500};
    request.verify_host = false;

    auto future = client.send_async(request);
    auto response = future.get();

    EXPECT_EQ(response.status_code, 201);
    EXPECT_EQ(response.body, "created");
    EXPECT_EQ(client.call_count(), 1);
    auto recorded_request = client.last_request();
    ASSERT_TRUE(recorded_request.has_value());
    EXPECT_EQ(recorded_request->method, alpaca::HttpMethod::POST);
    EXPECT_EQ(recorded_request->url, "https://example.com");
    EXPECT_EQ(recorded_request->body, "payload");
    EXPECT_EQ(recorded_request->timeout, std::chrono::milliseconds{1500});
    EXPECT_FALSE(recorded_request->verify_host);
}

} // namespace
