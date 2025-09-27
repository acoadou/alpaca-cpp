#pragma once

#include <deque>
#include <utility>
#include <vector>

#include "alpaca/HttpClient.hpp"

struct RecordedRequest {
    alpaca::HttpRequest request;
};

struct FakeHttpResponse {
    alpaca::HttpResponse response;
};

/// Simple fake implementation used to capture requests within unit tests.
class FakeHttpClient : public alpaca::HttpClient {
  public:
    FakeHttpClient() = default;

    void push_response(alpaca::HttpResponse response) {
        responses_.push_back(std::move(response));
    }

    [[nodiscard]] std::vector<RecordedRequest> const& requests() const {
        return requests_;
    }

    alpaca::HttpResponse send(alpaca::HttpRequest const& request) override {
        requests_.push_back(RecordedRequest{request});
        if (responses_.empty()) {
            return alpaca::HttpResponse{};
        }
        alpaca::HttpResponse response = std::move(responses_.front());
        responses_.pop_front();
        return response;
    }

  private:
    std::deque<alpaca::HttpResponse> responses_{};
    std::vector<RecordedRequest> requests_{};
};
