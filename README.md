# alpaca-cpp

Modern C++ client library for the [Alpaca Trade API](https://alpaca.markets/) inspired by the official C# SDK. The goal of
this project is to offer a strongly typed, fully documented trading surface that mirrors the capabilities of the official
client while embracing contemporary C++ idioms. The codebase builds a compiled library (static by default, or shared when
`BUILD_SHARED_LIBS` is enabled) that you link into your applications.

## Highlights

- **Comprehensive endpoint coverage** for accounts, account configuration, activities, orders (including advanced order
  classes), positions, assets, calendar/clock data, portfolio history and watchlists.
- **Domain specific clients** for trading, market data, and broker operations mirroring the layout of the official
  Alpaca SDKs, with environment presets for paper and live trading.
- **Market data helpers** for fetching latest trades/quotes, historical bars and per-symbol snapshots from Alpaca Market
  Data v2.
- **Typed request models** that perform lightweight validation before hitting the API and cursor ranges that iterate
  paginated endpoints while honouring `Retry-After` headers and applying exponential backoff with jitter.
- **Extensible transport** abstraction with a libcurl-based implementation bundled by default.
- **Secure by default** HTTPS requests with TLS peer and hostname verification, configurable CA bundles and
  opt-in overrides for custom environments.
- **Modern model layer** that maps JSON payloads to rich C++ structures using `std::optional`, `std::vector`, and chrono
  types where appropriate.
- **Robust testing** powered by GoogleTest/GoogleMock and a fake HTTP client that validates headers, query encoding and JSON
  serialization.
- **Synchronous by design** REST and streaming clients that you can wrap in your preferred executor, coroutine, or
  future abstraction when integrating with asynchronous pipelines.

## Endpoint coverage

| Domain           | REST | Streaming | Notes |
|------------------|:----:|:---------:|-------|
| Trading          | OK   | OK        | Orders, positions, activities, market clock, watchlists, and `trade_updates` / `account_updates` streams. |
| Market Data      | OK   | OK        | Trades, quotes, stock bars, snapshots, and automatic pagination via `PaginatedVectorRange`. |
| Broker / Connect | OK   | N/A       | Broker API for accounts, documents, transfers, journals, and banking relationships. |
| Options          | WIP  | OK        | REST aggregates/quotes/trades and options websocket feed; options trading APIs still outstanding. |
| News             | OK   | N/A       | REST `get_news` plus the `news_range` paginator; no public streaming endpoint. |
| Crypto           | OK   | OK        | REST aggregates/quotes/trades and dedicated crypto streaming feed. |

## Getting started

### Prerequisites

- A C++20 capable compiler (GCC 11+, Clang 13+, MSVC 19.3+).
- CMake 3.20 or newer.
- Development headers for libcurl (for example `libcurl4-openssl-dev` on Debian/Ubuntu or
  `brew install curl` on macOS).
- Development headers for OpenSSL (for example `libssl-dev` on Debian/Ubuntu or
  `brew install openssl` on macOS).
- Optionally, a system installation of [`nlohmann_json`](https://github.com/nlohmann/json). If it
  is not present, the build will automatically fetch version 3.11.2 during configuration so you do
  not need to vendor the single-header distribution manually.
- [ixwebsocket](https://github.com/machinezone/IXWebSocket) installed on the system or made
  available through your package manager. When it is not available, configure the build with
  `-DALPACA_FETCH_IXWEBSOCKET=ON` (the default) to download and build ixwebsocket as part of the
  project. Downstream packagers are encouraged to install the dependency ahead of time and set
  `-DALPACA_FETCH_IXWEBSOCKET=OFF` so the build fails fast if the system package is missing.

### Selecting an environment

The library exposes ready-made environments that mirror the official SDKs. Use `alpaca::Environments`
to choose paper or live trading and build a configuration that wires both REST and streaming
endpoints:

```cpp
alpaca::Configuration paper = alpaca::Configuration::FromEnvironment(
    alpaca::Environments::Paper(),
    "API_KEY",
    "SECRET"
);

// Domain clients can be used independently when you do not need the umbrella AlpacaClient.
alpaca::TradingClient trading(paper);
alpaca::MarketDataClient market_data(paper);
```

You can still override any field on the configuration after construction when targeting custom
deployments.

### Building and running the tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The test suite is optional; set `-DALPACA_BUILD_TESTS=OFF` when configuring if you
are packaging the library and do not want to download or build GoogleTest.

### Creating installable packages

For Debian or Ubuntu environments you can leverage the provided `Makefile`
target to produce both `.deb` and `.tar.gz` archives using CPack:

```bash
make package
```

The artifacts are generated under `build/` and can be installed with
`sudo dpkg -i build/alpaca-cpp-*.deb`.

### Consuming the library from another project

The project installs a complete CMake package so you can either reference the
sources directly or depend on an installed copy.

#### Using `find_package`

After installing the library (`cmake --install build` or by deploying one of
the generated packages) you can discover it from another build with:

```cmake
find_package(alpaca-cpp CONFIG REQUIRED)

add_executable(my_trader main.cpp)
target_link_libraries(my_trader PRIVATE alpaca::alpaca-cpp)
```

This approach reuses the exported targets, propagates include directories for
both the public headers and the generated `alpaca/version.hpp` file, and keeps
the user agent string aligned with the version of the package that is
installed.

#### Using `FetchContent`

If you prefer to build the dependency from source inside your project you can
still leverage [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html):

```cmake
include(FetchContent)

FetchContent_Declare(
  alpaca-cpp
  GIT_REPOSITORY https://github.com/acoadou/alpaca-cpp.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(alpaca-cpp)

add_executable(my_trader main.cpp)
target_link_libraries(my_trader PRIVATE alpaca::alpaca-cpp)
```

This approach keeps dependency management within CMake, ensures the correct
transitive include directories and compiler options are propagated, and allows
you to track a specific release or commit through the `GIT_TAG` field.

## Handling empty REST responses

Some Alpaca endpoints (for example, cancellation operations) return `204 No
Content`. The `alpaca::RestClient` now treats those payloads as missing data and
supports requesting `std::optional<T>` for such endpoints. When no body is
present the template returns `std::nullopt` instead of attempting to deserialize
an empty JSON object, which avoids conversion errors for primitive or container
types. Requests that expect raw `alpaca::Json` still receive an empty object for
compatibility, while default-constructible value types are defaulted when no
content is available.

## Rate limiting and error handling

All non-success responses raise `alpaca::ApiException`. In addition to the HTTP
status code and message, the exception now exposes the raw response headers via
`headers()` and parses numeric `Retry-After` hints into
`std::optional<std::chrono::seconds>` through `retry_after()`. When you receive
`429` or `503` responses you can inspect the optional delay to drive a
backoff strategy:

```cpp
try {
  client.cancel_order(order_id);
} catch (const alpaca::ApiException& ex) {
  if (ex.status_code() == 429 || ex.status_code() == 503) {
    if (const auto retry_after = ex.retry_after()) {
      std::this_thread::sleep_for(*retry_after);
    }
    // Re-dispatch the operation or surface a retryable error to the caller.
  }
}
```

Documenting the retry strategy in your application—backing off and respecting
`Retry-After`, or switching to an asynchronous queue for replays—helps you stay
within the published limits while keeping the SDK responsive to transient
outages.

## Usage examples

```cpp
#include <alpaca/AlpacaClient.hpp>
#include <alpaca/Chrono.hpp>

int main() {
  alpaca::Configuration config = alpaca::Configuration::FromEnvironment(
      alpaca::Environments::Paper(),
      "API_KEY",
      "SECRET_KEY");
  alpaca::AlpacaClient client(config);

  alpaca::Account account = client.trading().get_account();

  alpaca::StockBarsRequest intraday;
  intraday.timeframe = "1Min";
  intraday.start = alpaca::since(std::chrono::hours{1});
  for (auto const& bar : client.stock_bars_range("AAPL", intraday)) {
    // Process bar data across every page.
  }

  alpaca::NewsRequest news;
  news.symbols = {"AAPL"};
  news.limit = 5;
  for (auto const& article : client.news_range(news)) {
    // Consume the latest headlines without managing pagination manually.
  }

  alpaca::NewOrderRequest order;
  order.symbol = "AAPL";
  order.side = alpaca::OrderSide::BUY;
  order.type = alpaca::OrderType::LIMIT;
  order.time_in_force = alpaca::TimeInForce::DAY;
  order.quantity = "1";
  order.limit_price = "150";
  client.trading().submit_order(order);

  return 0;
}
```

### Streaming actions: reconnect and resubscribe

A full example is available in [`examples/StreamingResubscribe.cpp`](examples/StreamingResubscribe.cpp). It demonstrates how
to configure `alpaca::streaming::WebSocketClient` with a reconnect policy, replay subscriptions after reconnecting, and
receive order updates:

```cpp
alpaca::Configuration config = alpaca::Configuration::FromEnvironment(
    alpaca::Environments::Paper(),
    std::getenv("APCA_API_KEY_ID"),
    std::getenv("APCA_API_SECRET_KEY"));

alpaca::streaming::WebSocketClient socket(
    config.trading_stream_url,
    config.api_key_id,
    config.api_secret_key,
    alpaca::streaming::StreamFeed::Trading);

socket.set_reconnect_policy({
    std::chrono::milliseconds{250},
    std::chrono::seconds{15},
    2.0,
    std::chrono::milliseconds{250}});

socket.set_message_handler([](auto const& message, auto category) {
    if (category == alpaca::streaming::MessageCategory::OrderUpdate) {
        auto const& update = std::get<alpaca::streaming::OrderUpdateMessage>(message);
        std::cout << "event " << update.event << " for order "
                  << update.order.id << std::endl;
    }
});

socket.set_open_handler([&]() {
    socket.listen({"trade_updates", "account_updates"});
});

socket.connect();
```

### Intraday bars, limit order and `Retry-After` handling

[`examples/IntradayLimitOrder.cpp`](examples/IntradayLimitOrder.cpp) combines fetching intraday bars, placing a limit order,
and handling 429 responses using `retry_after()`:

```cpp
alpaca::MarketDataClient market(config);
alpaca::TradingClient trading(config);

alpaca::StockBarsRequest request;
request.timeframe = "1Min";
request.start = alpaca::since(std::chrono::hours{2});

for (auto const& bar : market.stock_bars_range("AAPL", request)) {
    std::cout << bar.timestamp << " close=" << bar.close << std::endl;
}

alpaca::NewOrderRequest order;
order.symbol = "AAPL";
order.side = alpaca::OrderSide::BUY;
order.type = alpaca::OrderType::LIMIT;
order.time_in_force = alpaca::TimeInForce::DAY;
order.quantity = "1";
order.limit_price = "150";

bool submitted = false;
while (!submitted) {
    try {
        trading.submit_order(order);
        submitted = true;
    } catch (alpaca::ApiException const& ex) {
        if (ex.status_code() == 429) {
            if (auto delay = ex.retry_after()) {
                std::this_thread::sleep_for(*delay);
                continue;
            }
        }
        throw;
    }
}
```

## Extensibility

Additional end-to-end samples are available under [`examples/`](examples), including
[`StreamingResubscribe.cpp`](examples/StreamingResubscribe.cpp) for trading stream management and
[`IntradayLimitOrder.cpp`](examples/IntradayLimitOrder.cpp) for intraday bars, limit orders, and `Retry-After` handling.

All REST operations are issued through the `alpaca::HttpClient` interface. The default implementation uses libcurl, but you can
provide any custom transport (Boost.Beast, cpp-httplib, proprietary stacks, …) by supplying your own implementation when
constructing `alpaca::AlpacaClient`.

When you stick with the bundled curl transport you can tune concurrency and redirect behaviour with
`alpaca::CurlHttpClientOptions`. The defaults favour safety by disabling automatic redirects and keeping a single reusable
easy handle, but you can opt into a small pool when higher throughput is required:

```cpp
alpaca::CurlHttpClientOptions options;
options.connection_pool_size = 4;
options.follow_redirects = true;        // Will still honour max_redirects and protocol restrictions
options.max_redirects = 3;

auto http_client = alpaca::create_default_http_client(options);
alpaca::TradingClient trading(config, http_client);
```

Redirects remain disabled by default to avoid leaking credentials toward untrusted hosts, so only enable them when you control
the upstream endpoints.

### Streaming

`alpaca::streaming::WebSocketClient` bundles robust reconnect behaviour by default. You can tweak the
backoff settings and control heartbeats through the new setters:

```cpp
alpaca::streaming::WebSocketClient socket(
    config.market_data_stream_url + "/stocks",
    config.api_key_id,
    config.api_secret_key,
    alpaca::streaming::StreamFeed::MarketData);

socket.set_reconnect_policy({
    std::chrono::milliseconds{500},  // initial delay
    std::chrono::seconds{10},        // max delay
    2.0,                             // multiplier
    std::chrono::milliseconds{250}   // jitter
});
socket.set_ping_interval(std::chrono::seconds{15});
```

The client automatically resubscribes after reconnecting and responds to server pings with a `pong`
message, so long-running feeds keep flowing without manual intervention.

Outbound messages queued while the socket is down are now bounded (1024 by default) to guard against accidental memory
growth. Use `set_pending_message_limit(0)` if you explicitly need an unbounded buffer or pick a tighter cap that suits your
traffic profile:

```cpp
socket.set_pending_message_limit(256);
```

### TLS configuration

The `alpaca::Configuration` structure exposes `verify_ssl`, `verify_hostname`, `ca_bundle_path`, and `ca_bundle_dir` fields to
control TLS behavior when using `CurlHttpClient`. They default to secure values that validate server certificates and
hostnames. These knobs make it possible to point the client at private certificate authorities or fully disable validation when
working inside controlled development environments.

## Dependencies

The build system fetches the following upstream dependencies when configuring the project:

- [nlohmann/json](https://github.com/nlohmann/json) for JSON serialization.
- [GoogleTest](https://github.com/google/googletest) and GoogleMock for unit testing.
- [libcurl](https://curl.se/libcurl/) for the default HTTP transport.
- [ixwebsocket](https://github.com/machinezone/IXWebSocket) for standalone,
  TLS-capable websocket connectivity. We prefer ixwebsocket over
  [uWebSockets](https://github.com/uNetworking/uWebSockets) because it manages
  its own worker thread and does not require integrating an external event
  loop (libuv, ASIO, etc.), which keeps the streaming API drop-in for
  applications without a pre-existing reactor. The build first attempts to
  resolve a system-provided `ixwebsocket::ixwebsocket` target via
  `find_package(ixwebsocket CONFIG)`. If the dependency is not installed and
  `ALPACA_FETCH_IXWEBSOCKET` remains enabled, version 11.3.3 is downloaded with
  CMake's `FetchContent` module and built alongside the SDK so packages include
  the websocket client automatically.

All code is written in modern C++20 and aims to stay close to the semantics and surface area of the official C# SDK while
remaining lightweight and easy to integrate into larger projects.

## License compliance

If you redistribute `alpaca-cpp` in binary form, remember that the library links
against OpenSSL. Downstream packages must include the OpenSSL and original
SSLeay license notices, which are reproduced in
[`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md), alongside the project
license. Including that file in your distribution keeps the attribution
requirements satisfied for OpenSSL and the other bundled dependencies.
