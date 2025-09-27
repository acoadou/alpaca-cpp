#pragma once

#include "alpaca/CurlHttpClientOptions.hpp"
#include "alpaca/HttpClient.hpp"

namespace alpaca {

/// Creates the default libcurl-backed HTTP client used by the SDK.
HttpClientPtr create_default_http_client();

/// Creates a libcurl-backed HTTP client using the provided options.
HttpClientPtr create_default_http_client(CurlHttpClientOptions const& options);

} // namespace alpaca
