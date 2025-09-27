#pragma once

#include "alpaca/CurlHttpClientOptions.hpp"
#include "alpaca/HttpClient.hpp"

namespace alpaca {

/// HTTP client implementation that delegates to libcurl.
class CurlHttpClient : public HttpClient {
  public:
    explicit CurlHttpClient(CurlHttpClientOptions options = {});
    ~CurlHttpClient() override;

    HttpResponse send(HttpRequest const& request) override;

  private:
    struct Impl;
    Impl *impl_;
};

} // namespace alpaca
