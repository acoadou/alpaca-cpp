#pragma once

#include <cstddef>

namespace alpaca {

/// Configuration for the libcurl-backed HTTP client.
struct CurlHttpClientOptions {
    /// Number of reusable libcurl easy handles kept in the pool.
    std::size_t connection_pool_size{1};

    /// Enables automatic redirect following. Disabled by default to avoid
    /// credential leakage toward untrusted hosts.
    bool follow_redirects{false};

    /// Maximum number of redirects followed when \c follow_redirects is true.
    long max_redirects{5};

    /// Restricts redirect protocols to HTTP(S) when following redirects.
    bool restrict_redirect_protocols{true};
};

} // namespace alpaca
