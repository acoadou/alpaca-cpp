# alpaca-cpp

Modern C++ client library for the [Alpaca Trade API](https://alpaca.markets/) inspired by the official C# SDK. The goal of
this project is to offer a strongly typed, fully documented trading surface that mirrors the capabilities of the official
client while embracing contemporary C++ idioms. The codebase builds a compiled library (static by default, or shared when
`BUILD_SHARED_LIBS` is enabled) that you link into your applications.

## Highlights

- **Comprehensive endpoint coverage** for accounts, account configuration, activities, equities/options/crypto/OTC
  orders, positions, assets, calendar/clock data, portfolio history and watchlists.
- **Domain specific clients** for trading, market data, and broker operations mirroring the layout of the official
  Alpaca SDKs, with environment presets for paper and live trading and full support for onboarding, funding, and
  managed portfolio flows.
- **Market data helpers** for fetching latest trades/quotes, historical bars, order books, corporate actions, market
  movers, single and multi-symbol snapshots, plus options snapshots and chains from Alpaca Market Data APIs.
- **Typed request models** that perform lightweight validation before hitting the API and cursor ranges that iterate
  paginated endpoints while honouring `Retry-After` headers and applying exponential backoff with jitter.
- **Extensible transport** abstraction with a libcurl-based implementation bundled by default.
- **Secure by default** HTTPS requests with TLS peer and hostname verification, configurable CA bundles and
  opt-in overrides for custom environments.
- **Modern model layer** that maps JSON payloads to rich C++ structures using `std::optional`, `std::vector`, and chrono
  types where appropriate.
- **Robust testing** powered by GoogleTest/GoogleMock and a fake HTTP client that validates headers, query encoding and JSON
  serialization.
- **Native async helpers** with `std::future`-based REST endpoints and websocket controls alongside the existing
  synchronous APIs, enabling seamless integration with executors, coroutines, or custom event loops.
- **First-class OAuth2 support** with helpers for PKCE generation, authorization URL construction, and token exchange for
  Alpaca Connect applications.

## Endpoint coverage

| Domain           | REST | Streaming | Notes |
|------------------|:----:|:---------:|-------|
| Trading          | OK   | OK        | Equities, options, crypto, and OTC order workflows, option analytics/contracts, positions, activities, clock, watchlists, and `trade_updates` / `account_updates` streams. |
| Market Data      | OK   | OK        | Trades, quotes, bars, order books, corporate actions, market movers, single/multi snapshots, options snapshots/chains, and automatic pagination via `PaginatedVectorRange`. |
| Broker / Connect | OK   | N/A       | Account onboarding, documents, transfers, journals, banking relationships, watchlists, rebalancing portfolios/subscriptions, and managed portfolio history. |
| Options          | OK   | OK        | REST aggregates/quotes/trades, option analytics (with greeks + strategy legs), contracts, and trading endpoints for submitting, replacing, and cancelling single or multi-leg options orders. |
| News             | OK   | OK        | REST `get_news`, the `news_range` paginator, and websocket `news` channels on the market data feed. |
| Crypto           | OK   | OK        | REST aggregates/quotes/trades, order books, and dedicated crypto streaming feed. |

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

### Filtering orders by side

`alpaca::ListOrdersRequest` exposes a `side` filter so you can target only buys or sells when
inspecting account history. Combine it with existing filters like `status`, `nested`, or specific
symbols to trim the payload down to just the legs you care about:

```cpp
alpaca::ListOrdersRequest history{};
history.status = alpaca::OrderStatusFilter::CLOSED;
history.side = alpaca::OrderSide::SELL;
history.nested = true;

for (alpaca::Order const& order : trading.list_orders(history)) {
    std::cout << order.id << " -> " << to_string(order.side) << '\n';
}
```

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

## Options trading and analytics walkthrough

Alpaca's options APIs expose both market data (chains, greeks, and strategy legs)
and trading workflows (single-leg and multi-leg order management). The
`TradingClient` and `MarketDataClient` surfaces in this library map those
capabilities directly, so you can discover contracts, analyse the greeks for a
potential spread, and submit/cancel combo orders without juggling raw HTTP calls
yourself.

```cpp
#include <iostream>
#include <stdexcept>

#include "alpaca/Configuration.hpp"
#include "alpaca/TradingClient.hpp"

alpaca::Configuration config = alpaca::Configuration::Paper("KEY", "SECRET");
alpaca::TradingClient trading(config);

// 1. Discover the option chain for a given underlying symbol and expiry.
alpaca::ListOptionContractsRequest chain{};
chain.underlying_symbols = {"AAPL"};
chain.expiry = "2024-01-19";         // Filter by expiration date
chain.type = "call";                 // Filter by option type
chain.strike_gte = "180";            // Strike filtering helpers
chain.strike_lte = "200";

alpaca::OptionContractsResponse contracts = trading.list_option_contracts(chain);
if (contracts.contracts.empty()) {
    throw std::runtime_error("No matching contracts");
}

// 2. Pull analytics (greeks, risk metrics, and strategy legs) for the legs you
//    care about. The analytics payload mirrors the REST response, so multi-leg
//    strategies will surface their individual legs automatically.
alpaca::ListOptionAnalyticsRequest analytics_request{};
for (alpaca::OptionContract const& contract : contracts.contracts) {
    analytics_request.symbols.push_back(contract.symbol);
}
analytics_request.include_greeks = true;
analytics_request.include_risk_parameters = true;

alpaca::OptionAnalyticsResponse analytics = trading.list_option_analytics(analytics_request);
for (alpaca::OptionAnalytics const& entry : analytics.analytics) {
    if (entry.greeks && entry.greeks->delta) {
        std::cout << entry.symbol << " Δ=" << *entry.greeks->delta << '\n';
    }
    for (alpaca::OptionStrategyLeg const& leg : entry.legs) {
        std::cout << "  leg: " << leg.symbol << " x" << leg.ratio << '\n';
    }
}

// 3. Submit a spread (multi-leg) order. Alpaca encodes complex strategies as
//    synthetic symbols – use the symbols returned by the contract/analytics
//    endpoints so the exchange interprets the combo correctly.
alpaca::NewOptionOrderRequest order{};
order.symbol = analytics.analytics.front().symbol; // e.g. a call spread combo
order.side = alpaca::OrderSide::BUY;
order.type = alpaca::OrderType::LIMIT;
order.time_in_force = alpaca::TimeInForce::DAY;
order.quantity = "1";                // Spreads are still sized with qty
order.limit_price = "2.45";          // Net debit/credit for the combo

alpaca::OptionOrder placed = trading.submit_option_order(order);

// 4. Adjust or unwind the position: replace, cancel, or cancel all.
alpaca::ReplaceOptionOrderRequest replace{};
replace.limit_price = "2.10";

placed = trading.replace_option_order(placed.id, replace);
trading.cancel_option_order(placed.id);

// Multi-leg orders return nested legs when you ask for nested results.
alpaca::ListOptionOrdersRequest history{};
history.nested = true;               // Expands combo legs in the response

std::vector<alpaca::OptionOrder> orders = trading.list_option_orders(history);
for (alpaca::OptionOrder const& existing : orders) {
    if (!existing.legs.empty()) {
        std::cout << existing.symbol << " has " << existing.legs.size() << " legs" << '\n';
    }
}
```

Key takeaways:

- **Chain discovery and filtering** – `ListOptionContractsRequest` supports
  filtering by underlying symbols, expiry, type, strike bounds, limit, and
  pagination tokens so you can iterate option chains.
- **Analytics and greeks** – `ListOptionAnalyticsRequest` toggles greeks and
  risk metrics (`delta`, `gamma`, implied volatility, breakevens, and more) and
  exposes the underlying strategy legs for multi-leg combos through
  `OptionStrategyLeg`.
- **Order lifecycle** – `submit_option_order`, `replace_option_order`,
  `cancel_option_order`, and `cancel_all_option_orders` all target the dedicated
  options endpoints and accept the same request/response models as equities.
  Multi-leg strategies use the synthetic symbols surfaced by the discovery
  endpoints, and nested responses give you the per-leg fill detail.

Pair the `TradingClient` with `MarketDataClient` helpers (snapshots, latest
quotes, or historical bars for the contract symbols you surfaced) when you need
intraday analytics alongside greeks.

## OAuth / Connect authorization

Applications integrating with [Alpaca Connect](https://alpaca.markets/docs/connect/) can use the
`alpaca::OAuthClient` utilities bundled with the SDK to drive the OAuth 2.0
authorization code flow with PKCE. The helpers produce a compliant verifier,
construct the authorization URL, and exchange authorization/refresh tokens using
the bundled HTTP transport:

```cpp
#include "alpaca/HttpClientFactory.hpp"
#include "alpaca/OAuth.hpp"

alpaca::PkcePair pkce = alpaca::GeneratePkcePair();

alpaca::AuthorizationUrlRequest auth{};
auth.authorize_endpoint = "https://app.alpaca.markets/oauth/authorize";
auth.client_id = "YOUR_CLIENT_ID";
auth.redirect_uri = "https://example.com/callback";
auth.code_challenge = pkce.challenge;
auth.scope = "account trading";

std::cout << "Open this URL: " << alpaca::BuildAuthorizationUrl(auth) << '\n';

auto http = alpaca::create_default_http_client({});
alpaca::OAuthClient oauth{"https://broker-api.alpaca.markets/oauth/token", http};

alpaca::AuthorizationCodeTokenRequest request{};
request.client_id = "YOUR_CLIENT_ID";
request.redirect_uri = "https://example.com/callback";
request.code = "CODE_FROM_REDIRECT";
request.code_verifier = pkce.verifier;

alpaca::OAuthTokenResponse tokens = oauth.ExchangeAuthorizationCode(request);

// Use bearer authentication for subsequent requests.
alpaca::Configuration config;
config.broker_base_url = "https://broker-api.alpaca.markets";
tokens.apply(config);
```

Refer to [`examples/oauth_pkce.cpp`](examples/oauth_pkce.cpp) for an
end-to-end console sample that guides the user through the flow and stores the
resulting bearer token on an `alpaca::Configuration` instance.

## Asynchronous REST and streaming usage

All REST verbs on `alpaca::RestClient` now expose `_async` variants that return
`std::future<T>`. These helpers reuse the existing retry, rate limit, and JSON
handling paths while dispatching work on a background task so you can compose
them with executors, coroutines, or custom schedulers:

```cpp
auto http = alpaca::create_default_http_client({});
alpaca::Configuration config = alpaca::Configuration::Paper("KEY", "SECRET");
alpaca::RestClient rest(config, http, config.trading_base_url);

std::future<alpaca::Order> order_fut = rest.post_async<alpaca::Order>(
    "/v2/orders",
    alpaca::Json{{"symbol", "AAPL"},
                 {"qty", "10"},
                 {"side", "buy"},
                 {"type", "market"},
                 {"time_in_force", "day"}});

// Block, await, or transform the future.
alpaca::Order order = order_fut.get();
```

Websocket operations mirror this pattern with asynchronous helpers for
connecting, subscribing, and sending payloads:

```cpp
alpaca::streaming::WebSocketClient client(config.market_data_stream_url,
                                          config.api_key_id,
                                          config.api_secret_key);

alpaca::streaming::MarketSubscription sub{};
sub.trades.push_back("AAPL");

auto subscribe_fut = client.subscribe_async(sub);
subscribe_fut.get();
```

Custom HTTP transports can override `alpaca::HttpClient::send_async` to hook the
SDK into existing event loops rather than relying on the default thread-based
dispatcher.

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

All non-success responses raise `alpaca::ApiException` or one of its more
specific subclasses. The hierarchy allows callers to handle failures with
additional precision:

* `alpaca::AuthenticationException` – Invalid or missing credentials.
* `alpaca::PermissionException` – Authenticated but lacking the required scopes.
* `alpaca::NotFoundException` – Requested resource does not exist.
* `alpaca::RateLimitException` – Too many requests were issued; examine
  `retry_after()` when present.
* `alpaca::ValidationException` – The request payload failed validation.
* `alpaca::ClientException` – Other 4xx errors that are not covered above.
* `alpaca::ServerException` – 5xx server side failures.

All of these derive from `alpaca::ApiException`, which still exposes the raw
response headers via `headers()` and parses numeric `Retry-After` hints into
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
  intraday.timeframe = alpaca::TimeFrame::minute();
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

### Selecting an equities market data plan

The Market Data API exposes different equities feeds depending on your data plan.
By default `alpaca::MarketDataClient` works in auto-detection mode: it inspects
the configured URLs to decide whether to call the IEX (free) or SIP (consolidated)
feed and falls back to IEX when no hints are present. The chosen plan is applied
consistently to every equities request so the generated URLs always include an
explicit `feed` query parameter.

You can lock the plan explicitly through `alpaca::Configuration::market_data_plan`.
When an IEX-only configuration attempts to use the SIP feed, the client throws an
exception with a clear message before issuing any HTTP request.

```cpp
alpaca::Configuration config = alpaca::Configuration::Paper("KEY", "SECRET");
config.market_data_plan = alpaca::MarketDataPlan::SIP; // opt into consolidated SIP data

alpaca::MarketDataClient market(config);
alpaca::LatestStockTrade trade = market.get_latest_stock_trade("AAPL");
// -> requests https://data.sip.alpaca.markets/...&feed=sip (or your custom base URL)
```

If your Alpaca environment uses a proxy or custom hostname, include `sip` or
`iex` in the URL to help the auto-detector pick the correct plan, or set the
plan explicitly as shown above.

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

### Streaming news headlines

[`examples/NewsStream.cpp`](examples/NewsStream.cpp) shows how to connect to the market data websocket feed and subscribe to
the `news` channel to receive headlines in real time:

```cpp
alpaca::streaming::WebSocketClient socket(
    config.market_data_stream_url,
    config.api_key_id,
    config.api_secret_key,
    alpaca::streaming::StreamFeed::MarketData);

socket.set_message_handler(
    [](alpaca::streaming::StreamMessage const& message, alpaca::streaming::MessageCategory category) {
        if (category != alpaca::streaming::MessageCategory::News) {
            return;
        }

        auto const& article = std::get<alpaca::streaming::NewsMessage>(message);
        std::cout << "[news]";
        for (auto const& symbol : article.symbols) {
            std::cout << ' ' << symbol;
        }
        std::cout << " :: " << article.headline << std::endl;
    });

socket.set_open_handler([&socket]() {
    alpaca::streaming::MarketSubscription subscription;
    subscription.news = {"*"};
    socket.subscribe(subscription);
});

socket.connect();
```

The websocket client already understands `news` payloads via the
`alpaca::streaming::MarketSubscription::news` field and parses them into
`alpaca::streaming::NewsMessage` instances (see
[`include/alpaca/Streaming.hpp`](include/alpaca/Streaming.hpp) and
[`src/Streaming.cpp`](src/Streaming.cpp)). Automated coverage exercises the
behaviour through `StreamingTest.RoutesNewsMessages` and the news serialization
tests under `tests/ModelSerializationTest.cpp` to make sure the payloads are
decoded as expected.

### Intraday bars, limit order and `Retry-After` handling

[`examples/IntradayLimitOrder.cpp`](examples/IntradayLimitOrder.cpp) combines fetching intraday bars, placing a limit order,
and handling 429 responses using `retry_after()`:

```cpp
alpaca::MarketDataClient market(config);
alpaca::TradingClient trading(config);

alpaca::StockBarsRequest request;
request.timeframe = alpaca::TimeFrame::minute();
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

#### Automatic REST backfill for sequence gaps

`alpaca::streaming::BackfillCoordinator` bridges sequence gaps observed on the websocket connection with historical REST
queries. When `alpaca::streaming::WebSocketClient` notices a discontinuity it invokes the coordinator, which uses
`alpaca::MarketDataClient` to retrieve the missing trades or bars and hands them back through optional replay handlers.

```cpp
#include <alpaca/BackfillCoordinator.hpp>

alpaca::MarketDataClient market_client(config);

auto coordinator = std::make_shared<alpaca::streaming::BackfillCoordinator>(
    market_client,
    alpaca::streaming::StreamFeed::MarketData);

coordinator->set_trade_replay_handler([](std::string const& symbol,
                                         std::vector<alpaca::StockTrade> const& trades) {
    for (auto const& trade : trades) {
        std::cout << "replayed trade for " << symbol << " at " << trade.price << '\n';
    }
});

socket.enable_automatic_backfill(coordinator);
```

When no custom `SequenceGapPolicy` is provided the helper installs default extractors for the `S` (symbol) and `i` (sequence)
fields used by Alpaca's market data feeds. You can disable automatic backfills at any time through
`socket.disable_automatic_backfill()`. Crypto feeds require a `BackfillCoordinator::Options::crypto_feed` hint describing the
REST feed (`"us"`, `"global"`, …), while equities and options default to one-minute bars when replaying aggregates.

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
