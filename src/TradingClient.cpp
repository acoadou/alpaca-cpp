#include "alpaca/TradingClient.hpp"

#include <utility>

#include "alpaca/HttpClientFactory.hpp"
#include "alpaca/Json.hpp"

namespace alpaca {
namespace {
HttpClientPtr ensure_http_client(HttpClientPtr& client) {
    if (!client) {
        client = create_default_http_client();
    }
    return client;
}

Json to_json_payload(AccountConfigurationUpdateRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(NewOrderRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(ReplaceOrderRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateWatchlistRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(UpdateWatchlistRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json symbol_payload(std::string const& symbol) {
    return Json{
        {"symbol", symbol}
    };
}
} // namespace

TradingClient::TradingClient(Configuration const& config, HttpClientPtr http_client)
  : rest_client_(config, ensure_http_client(http_client), config.trading_base_url) {
}

TradingClient::TradingClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                             HttpClientPtr http_client)
  : TradingClient(Configuration::FromEnvironment(environment, std::move(api_key_id), std::move(api_secret_key)),
                  std::move(http_client)) {
}

Account TradingClient::get_account() {
    return rest_client_.get<Account>("/v2/account");
}

AccountConfiguration TradingClient::get_account_configuration() {
    return rest_client_.get<AccountConfiguration>("/v2/account/configurations");
}

AccountConfiguration TradingClient::update_account_configuration(AccountConfigurationUpdateRequest const& request) {
    return rest_client_.patch<AccountConfiguration>("/v2/account/configurations", to_json_payload(request));
}

std::vector<Position> TradingClient::list_positions() {
    return rest_client_.get<std::vector<Position>>("/v2/positions");
}

Position TradingClient::get_position(std::string const& symbol) {
    return rest_client_.get<Position>("/v2/positions/" + symbol);
}

Position TradingClient::close_position(std::string const& symbol, ClosePositionRequest const& request) {
    return rest_client_.del<Position>("/v2/positions/" + symbol, request.to_query_params());
}

std::vector<Order> TradingClient::list_orders(ListOrdersRequest const& request) {
    return rest_client_.get<std::vector<Order>>("/v2/orders", request.to_query_params());
}

Order TradingClient::get_order(std::string const& order_id) {
    return rest_client_.get<Order>("/v2/orders/" + order_id);
}

Order TradingClient::get_order_by_client_order_id(std::string const& client_order_id) {
    return rest_client_.get<Order>("/v2/orders:by_client_order_id", {
                                                                        {"client_order_id", client_order_id}
    });
}

void TradingClient::cancel_order(std::string const& order_id) {
    rest_client_.del<void>("/v2/orders/" + order_id);
}

std::vector<CancelledOrderId> TradingClient::cancel_all_orders() {
    return rest_client_.del<std::vector<CancelledOrderId>>("/v2/orders");
}

Order TradingClient::submit_order(NewOrderRequest const& request) {
    return rest_client_.post<Order>("/v2/orders", to_json_payload(request));
}

Order TradingClient::replace_order(std::string const& order_id, ReplaceOrderRequest const& request) {
    return rest_client_.patch<Order>("/v2/orders/" + order_id, to_json_payload(request));
}

Clock TradingClient::get_clock() {
    return rest_client_.get<Clock>("/v2/clock");
}

std::vector<CalendarDay> TradingClient::get_calendar(CalendarRequest const& request) {
    return rest_client_.get<std::vector<CalendarDay>>("/v2/calendar", request.to_query_params());
}

std::vector<Asset> TradingClient::list_assets(ListAssetsRequest const& request) {
    return rest_client_.get<std::vector<Asset>>("/v2/assets", request.to_query_params());
}

Asset TradingClient::get_asset(std::string const& symbol) {
    return rest_client_.get<Asset>("/v2/assets/" + symbol);
}

std::vector<AccountActivity> TradingClient::get_account_activities(AccountActivitiesRequest const& request) {
    return rest_client_.get<std::vector<AccountActivity>>("/v2/account/activities", request.to_query_params());
}

PortfolioHistory TradingClient::get_portfolio_history(PortfolioHistoryRequest const& request) {
    return rest_client_.get<PortfolioHistory>("/v2/account/portfolio/history", request.to_query_params());
}

std::vector<Watchlist> TradingClient::list_watchlists() {
    return rest_client_.get<std::vector<Watchlist>>("/v2/watchlists");
}

Watchlist TradingClient::get_watchlist(std::string const& id) {
    return rest_client_.get<Watchlist>("/v2/watchlists/" + id);
}

Watchlist TradingClient::get_watchlist_by_name(std::string const& name) {
    return rest_client_.get<Watchlist>("/v2/watchlists:by_name", {
                                                                     {"name", name}
    });
}

Watchlist TradingClient::create_watchlist(CreateWatchlistRequest const& request) {
    return rest_client_.post<Watchlist>("/v2/watchlists", to_json_payload(request));
}

Watchlist TradingClient::update_watchlist(std::string const& id, UpdateWatchlistRequest const& request) {
    return rest_client_.put<Watchlist>("/v2/watchlists/" + id, to_json_payload(request));
}

Watchlist TradingClient::add_asset_to_watchlist(std::string const& id, std::string const& symbol) {
    return rest_client_.post<Watchlist>("/v2/watchlists/" + id, symbol_payload(symbol));
}

Watchlist TradingClient::remove_asset_from_watchlist(std::string const& id, std::string const& symbol) {
    return rest_client_.del<Watchlist>("/v2/watchlists/" + id + "/" + symbol);
}

void TradingClient::delete_watchlist(std::string const& id) {
    rest_client_.del<void>("/v2/watchlists/" + id);
}

} // namespace alpaca
