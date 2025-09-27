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

Json to_json_payload(NewCryptoOrderRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(NewOtcOrderRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(ReplaceCryptoOrderRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(ReplaceOtcOrderRequest const& request) {
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

std::vector<OptionPosition> TradingClient::list_option_positions() {
    return rest_client_.get<std::vector<OptionPosition>>("/v2/options/positions");
}

OptionPosition TradingClient::get_option_position(std::string const& symbol) {
    return rest_client_.get<OptionPosition>("/v2/options/positions/" + symbol);
}

OptionPosition TradingClient::close_option_position(std::string const& symbol,
                                                    CloseOptionPositionRequest const& request) {
    return rest_client_.del<OptionPosition>("/v2/options/positions/" + symbol, request.to_query_params());
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

std::vector<OptionOrder> TradingClient::list_option_orders(ListOptionOrdersRequest const& request) {
    return rest_client_.get<std::vector<OptionOrder>>("/v2/options/orders", request.to_query_params());
}

OptionOrder TradingClient::get_option_order(std::string const& order_id) {
    return rest_client_.get<OptionOrder>("/v2/options/orders/" + order_id);
}

OptionOrder TradingClient::get_option_order_by_client_order_id(std::string const& client_order_id) {
    return rest_client_.get<OptionOrder>("/v2/options/orders:by_client_order_id",
                                         {{"client_order_id", client_order_id}});
}

void TradingClient::cancel_option_order(std::string const& order_id) {
    rest_client_.del<void>("/v2/options/orders/" + order_id);
}

std::vector<OptionCancelledOrderId> TradingClient::cancel_all_option_orders() {
    return rest_client_.del<std::vector<OptionCancelledOrderId>>("/v2/options/orders");
}

OptionOrder TradingClient::submit_option_order(NewOptionOrderRequest const& request) {
    return rest_client_.post<OptionOrder>("/v2/options/orders", to_json_payload(request));
}

OptionOrder TradingClient::replace_option_order(std::string const& order_id,
                                                ReplaceOptionOrderRequest const& request) {
    return rest_client_.patch<OptionOrder>("/v2/options/orders/" + order_id, to_json_payload(request));
}

std::vector<CryptoOrder> TradingClient::list_crypto_orders(ListCryptoOrdersRequest request) {
    request.asset_class = AssetClass::CRYPTO;
    return rest_client_.get<std::vector<CryptoOrder>>("/v2/crypto/orders", request.to_query_params());
}

CryptoOrder TradingClient::get_crypto_order(std::string const& order_id) {
    return rest_client_.get<CryptoOrder>("/v2/crypto/orders/" + order_id);
}

CryptoOrder TradingClient::get_crypto_order_by_client_order_id(std::string const& client_order_id) {
    return rest_client_.get<CryptoOrder>("/v2/crypto/orders:by_client_order_id",
                                         {{"client_order_id", client_order_id}});
}

void TradingClient::cancel_crypto_order(std::string const& order_id) {
    rest_client_.del<void>("/v2/crypto/orders/" + order_id);
}

std::vector<CryptoCancelledOrderId> TradingClient::cancel_all_crypto_orders() {
    return rest_client_.del<std::vector<CryptoCancelledOrderId>>("/v2/crypto/orders");
}

CryptoOrder TradingClient::submit_crypto_order(NewCryptoOrderRequest const& request) {
    return rest_client_.post<CryptoOrder>("/v2/crypto/orders", to_json_payload(request));
}

CryptoOrder TradingClient::replace_crypto_order(std::string const& order_id,
                                               ReplaceCryptoOrderRequest const& request) {
    return rest_client_.patch<CryptoOrder>("/v2/crypto/orders/" + order_id, to_json_payload(request));
}

std::vector<OtcOrder> TradingClient::list_otc_orders(ListOtcOrdersRequest request) {
    return rest_client_.get<std::vector<OtcOrder>>("/v2/otc/orders", request.to_query_params());
}

OtcOrder TradingClient::get_otc_order(std::string const& order_id) {
    return rest_client_.get<OtcOrder>("/v2/otc/orders/" + order_id);
}

OtcOrder TradingClient::get_otc_order_by_client_order_id(std::string const& client_order_id) {
    return rest_client_.get<OtcOrder>("/v2/otc/orders:by_client_order_id",
                                      {{"client_order_id", client_order_id}});
}

void TradingClient::cancel_otc_order(std::string const& order_id) {
    rest_client_.del<void>("/v2/otc/orders/" + order_id);
}

std::vector<OtcCancelledOrderId> TradingClient::cancel_all_otc_orders() {
    return rest_client_.del<std::vector<OtcCancelledOrderId>>("/v2/otc/orders");
}

OtcOrder TradingClient::submit_otc_order(NewOtcOrderRequest const& request) {
    return rest_client_.post<OtcOrder>("/v2/otc/orders", to_json_payload(request));
}

OtcOrder TradingClient::replace_otc_order(std::string const& order_id, ReplaceOtcOrderRequest const& request) {
    return rest_client_.patch<OtcOrder>("/v2/otc/orders/" + order_id, to_json_payload(request));
}

OptionContractsResponse TradingClient::list_option_contracts(ListOptionContractsRequest const& request) {
    return rest_client_.get<OptionContractsResponse>("/v2/options/contracts", request.to_query_params());
}

OptionContract TradingClient::get_option_contract(std::string const& symbol) {
    return rest_client_.get<OptionContract>("/v2/options/contracts/" + symbol);
}

OptionAnalyticsResponse TradingClient::list_option_analytics(ListOptionAnalyticsRequest const& request) {
    return rest_client_.get<OptionAnalyticsResponse>("/v2/options/analytics", request.to_query_params());
}

OptionAnalytics TradingClient::get_option_analytics(std::string const& symbol) {
    return rest_client_.get<OptionAnalytics>("/v2/options/analytics/" + symbol);
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
