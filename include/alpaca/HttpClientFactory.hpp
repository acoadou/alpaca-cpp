#pragma once

#include "alpaca/HttpClient.hpp"

namespace alpaca {

/// Creates the default libcurl-backed HTTP client used by the SDK.
HttpClientPtr create_default_http_client();

} // namespace alpaca
