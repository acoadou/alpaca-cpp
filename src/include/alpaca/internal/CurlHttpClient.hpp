#pragma once

#include "alpaca/HttpClient.hpp"

namespace alpaca {

/// HTTP client implementation that delegates to libcurl.
class CurlHttpClient : public HttpClient {
 public:
  CurlHttpClient();
  ~CurlHttpClient() override;

  HttpResponse send(const HttpRequest& request) override;

 private:
  struct Impl;
  Impl* impl_;
};

}  // namespace alpaca

