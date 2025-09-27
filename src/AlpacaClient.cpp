#include "alpaca/AlpacaClient.hpp"
#include "alpaca/HttpClientFactory.hpp"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace alpaca {
namespace {
HttpClientPtr ensure_http_client(HttpClientPtr& client) {
    if (!client) {
        client = create_default_http_client();
    }
    return client;
}

std::string make_data_beta_base_url(std::string const& data_base_url) {
    constexpr std::string_view v2_suffix{"/v2"};
    if (data_base_url.ends_with(v2_suffix)) {
        return data_base_url.substr(0, data_base_url.size() - v2_suffix.size()) + "/v1beta1";
    }
    if (data_base_url.ends_with('/')) {
        return data_base_url + "v1beta1";
    }
    return data_base_url + "/v1beta1";
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

Json to_json_payload(CreateBrokerAccountRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(UpdateBrokerAccountRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateAccountDocumentRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateTransferRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateJournalRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateAchRelationshipRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateWireRelationshipRequest const& request) {
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

AlpacaClient::AlpacaClient(Configuration config, HttpClientPtr http_client)
  : trading_client_(config, ensure_http_client(http_client), config.trading_base_url),
    data_client_(config, ensure_http_client(http_client), config.data_base_url),
    data_beta_client_(config, ensure_http_client(http_client), make_data_beta_base_url(config.data_base_url)),
    broker_client_(config, ensure_http_client(http_client), config.broker_base_url) {
}

Account AlpacaClient::get_account() {
    return trading_client_.get<Account>("/v2/account");
}

AccountConfiguration AlpacaClient::get_account_configuration() {
    return trading_client_.get<AccountConfiguration>("/v2/account/configurations");
}

AccountConfiguration AlpacaClient::update_account_configuration(AccountConfigurationUpdateRequest const& request) {
    return trading_client_.patch<AccountConfiguration>("/v2/account/configurations", to_json_payload(request));
}

std::vector<Position> AlpacaClient::list_positions() {
    return trading_client_.get<std::vector<Position>>("/v2/positions");
}

Position AlpacaClient::get_position(std::string const& symbol) {
    return trading_client_.get<Position>("/v2/positions/" + symbol);
}

Position AlpacaClient::close_position(std::string const& symbol, ClosePositionRequest const& request) {
    return trading_client_.del<Position>("/v2/positions/" + symbol, request.to_query_params());
}

std::vector<Order> AlpacaClient::list_orders(ListOrdersRequest const& request) {
    return trading_client_.get<std::vector<Order>>("/v2/orders", request.to_query_params());
}

Order AlpacaClient::get_order(std::string const& order_id) {
    return trading_client_.get<Order>("/v2/orders/" + order_id);
}

Order AlpacaClient::get_order_by_client_order_id(std::string const& client_order_id) {
    return trading_client_.get<Order>("/v2/orders:by_client_order_id", {
                                                                           {"client_order_id", client_order_id}
    });
}

void AlpacaClient::cancel_order(std::string const& order_id) {
    trading_client_.del<void>("/v2/orders/" + order_id);
}

std::vector<CancelledOrderId> AlpacaClient::cancel_all_orders() {
    return trading_client_.del<std::vector<CancelledOrderId>>("/v2/orders");
}

Order AlpacaClient::submit_order(NewOrderRequest const& request) {
    return trading_client_.post<Order>("/v2/orders", to_json_payload(request));
}

Order AlpacaClient::replace_order(std::string const& order_id, ReplaceOrderRequest const& request) {
    return trading_client_.patch<Order>("/v2/orders/" + order_id, to_json_payload(request));
}

Clock AlpacaClient::get_clock() {
    return trading_client_.get<Clock>("/v2/clock");
}

std::vector<CalendarDay> AlpacaClient::get_calendar(CalendarRequest const& request) {
    return trading_client_.get<std::vector<CalendarDay>>("/v2/calendar", request.to_query_params());
}

std::vector<Asset> AlpacaClient::list_assets(ListAssetsRequest const& request) {
    return trading_client_.get<std::vector<Asset>>("/v2/assets", request.to_query_params());
}

Asset AlpacaClient::get_asset(std::string const& symbol) {
    return trading_client_.get<Asset>("/v2/assets/" + symbol);
}

std::vector<AccountActivity> AlpacaClient::get_account_activities(AccountActivitiesRequest const& request) {
    return trading_client_.get<std::vector<AccountActivity>>("/v2/account/activities", request.to_query_params());
}

PortfolioHistory AlpacaClient::get_portfolio_history(PortfolioHistoryRequest const& request) {
    return trading_client_.get<PortfolioHistory>("/v2/account/portfolio/history", request.to_query_params());
}

std::vector<Watchlist> AlpacaClient::list_watchlists() {
    return trading_client_.get<std::vector<Watchlist>>("/v2/watchlists");
}

Watchlist AlpacaClient::get_watchlist(std::string const& id) {
    return trading_client_.get<Watchlist>("/v2/watchlists/" + id);
}

Watchlist AlpacaClient::get_watchlist_by_name(std::string const& name) {
    return trading_client_.get<Watchlist>("/v2/watchlists:by_name", {
                                                                        {"name", name}
    });
}

Watchlist AlpacaClient::create_watchlist(CreateWatchlistRequest const& request) {
    return trading_client_.post<Watchlist>("/v2/watchlists", to_json_payload(request));
}

Watchlist AlpacaClient::update_watchlist(std::string const& id, UpdateWatchlistRequest const& request) {
    return trading_client_.put<Watchlist>("/v2/watchlists/" + id, to_json_payload(request));
}

Watchlist AlpacaClient::add_asset_to_watchlist(std::string const& id, std::string const& symbol) {
    return trading_client_.post<Watchlist>("/v2/watchlists/" + id, symbol_payload(symbol));
}

Watchlist AlpacaClient::remove_asset_from_watchlist(std::string const& id, std::string const& symbol) {
    return trading_client_.del<Watchlist>("/v2/watchlists/" + id + "/" + symbol);
}

void AlpacaClient::delete_watchlist(std::string const& id) {
    trading_client_.del<void>("/v2/watchlists/" + id);
}

LatestStockTrade AlpacaClient::get_latest_stock_trade(std::string const& symbol) {
    return data_client_.get<LatestStockTrade>("stocks/" + symbol + "/trades/latest");
}

LatestStockQuote AlpacaClient::get_latest_stock_quote(std::string const& symbol) {
    return data_client_.get<LatestStockQuote>("stocks/" + symbol + "/quotes/latest");
}

StockBars AlpacaClient::get_stock_bars(std::string const& symbol, StockBarsRequest const& request) {
    return data_client_.get<StockBars>("stocks/" + symbol + "/bars", request.to_query_params());
}

std::vector<StockBar> AlpacaClient::get_all_stock_bars(std::string const& symbol, StockBarsRequest request) {
    std::vector<StockBar> all_bars;
    std::optional<std::string> next_page = request.page_token;

    while (true) {
        if (next_page.has_value()) {
            request.page_token = next_page;
        } else {
            request.page_token.reset();
        }

        auto const page = get_stock_bars(symbol, request);
        all_bars.insert(all_bars.end(), page.bars.begin(), page.bars.end());

        next_page = page.next_page_token;
        if (!next_page.has_value()) {
            break;
        }
    }

    return all_bars;
}

StockSnapshot AlpacaClient::get_stock_snapshot(std::string const& symbol) {
    return data_client_.get<StockSnapshot>("stocks/" + symbol + "/snapshot");
}

NewsResponse AlpacaClient::get_news(QueryParams const& params) {
    return data_beta_client_.get<NewsResponse>("news", params);
}

CorporateActionAnnouncementsResponse AlpacaClient::get_corporate_announcements(QueryParams const& params) {
    return data_beta_client_.get<CorporateActionAnnouncementsResponse>("corporate-actions/announcements", params);
}

CorporateActionEventsResponse AlpacaClient::get_corporate_actions(QueryParams const& params) {
    return data_beta_client_.get<CorporateActionEventsResponse>("corporate-actions/events", params);
}

MultiStockBars AlpacaClient::get_stock_aggregates(QueryParams const& params) {
    return data_client_.get<MultiStockBars>("stocks/bars", params);
}

MultiStockQuotes AlpacaClient::get_stock_quotes(QueryParams const& params) {
    return data_client_.get<MultiStockQuotes>("stocks/quotes", params);
}

MultiStockTrades AlpacaClient::get_stock_trades(QueryParams const& params) {
    return data_client_.get<MultiStockTrades>("stocks/trades", params);
}

MultiOptionBars AlpacaClient::get_option_aggregates(QueryParams const& params) {
    return data_beta_client_.get<MultiOptionBars>("options/bars", params);
}

MultiOptionQuotes AlpacaClient::get_option_quotes(QueryParams const& params) {
    return data_beta_client_.get<MultiOptionQuotes>("options/quotes", params);
}

MultiOptionTrades AlpacaClient::get_option_trades(QueryParams const& params) {
    return data_beta_client_.get<MultiOptionTrades>("options/trades", params);
}

MultiCryptoBars AlpacaClient::get_crypto_aggregates(QueryParams const& params) {
    return data_beta_client_.get<MultiCryptoBars>("crypto/bars", params);
}

MultiCryptoQuotes AlpacaClient::get_crypto_quotes(QueryParams const& params) {
    return data_beta_client_.get<MultiCryptoQuotes>("crypto/quotes", params);
}

MultiCryptoTrades AlpacaClient::get_crypto_trades(QueryParams const& params) {
    return data_beta_client_.get<MultiCryptoTrades>("crypto/trades", params);
}

BrokerAccountsPage AlpacaClient::list_broker_accounts(ListBrokerAccountsRequest const& request) {
    return broker_client_.get<BrokerAccountsPage>("/v1/accounts", request.to_query_params());
}

BrokerAccount AlpacaClient::get_broker_account(std::string const& account_id) {
    return broker_client_.get<BrokerAccount>("/v1/accounts/" + account_id);
}

BrokerAccount AlpacaClient::create_broker_account(CreateBrokerAccountRequest const& request) {
    return broker_client_.post<BrokerAccount>("/v1/accounts", to_json_payload(request));
}

BrokerAccount AlpacaClient::update_broker_account(std::string const& account_id,
                                                  UpdateBrokerAccountRequest const& request) {
    return broker_client_.patch<BrokerAccount>("/v1/accounts/" + account_id, to_json_payload(request));
}

void AlpacaClient::delete_broker_account(std::string const& account_id) {
    broker_client_.post<void>("/v1/accounts/" + account_id + "/actions/close", Json::object());
}

std::vector<AccountDocument> AlpacaClient::list_account_documents(std::string const& account_id) {
    return broker_client_.get<std::vector<AccountDocument>>("/v1/accounts/" + account_id + "/documents");
}

AccountDocument AlpacaClient::upload_account_document(std::string const& account_id,
                                                      CreateAccountDocumentRequest const& request) {
    return broker_client_.post<AccountDocument>("/v1/accounts/" + account_id + "/documents", to_json_payload(request));
}

TransfersPage AlpacaClient::list_account_transfers(std::string const& account_id, ListTransfersRequest const& request) {
    return broker_client_.get<TransfersPage>("/v1/accounts/" + account_id + "/transfers", request.to_query_params());
}

Transfer AlpacaClient::create_account_transfer(std::string const& account_id, CreateTransferRequest const& request) {
    return broker_client_.post<Transfer>("/v1/accounts/" + account_id + "/transfers", to_json_payload(request));
}

Transfer AlpacaClient::get_transfer(std::string const& transfer_id) {
    return broker_client_.get<Transfer>("/v1/transfers/" + transfer_id);
}

void AlpacaClient::cancel_transfer(std::string const& account_id, std::string const& transfer_id) {
    broker_client_.del<void>("/v1/accounts/" + account_id + "/transfers/" + transfer_id);
}

JournalsPage AlpacaClient::list_journals(ListJournalsRequest const& request) {
    return broker_client_.get<JournalsPage>("/v1/journals", request.to_query_params());
}

Journal AlpacaClient::create_journal(CreateJournalRequest const& request) {
    return broker_client_.post<Journal>("/v1/journals", to_json_payload(request));
}

Journal AlpacaClient::get_journal(std::string const& journal_id) {
    return broker_client_.get<Journal>("/v1/journals/" + journal_id);
}

void AlpacaClient::cancel_journal(std::string const& journal_id) {
    broker_client_.del<void>("/v1/journals/" + journal_id);
}

BankRelationshipsPage AlpacaClient::list_ach_relationships(std::string const& account_id) {
    return broker_client_.get<BankRelationshipsPage>("/v1/accounts/" + account_id + "/ach_relationships");
}

BankRelationship AlpacaClient::create_ach_relationship(std::string const& account_id,
                                                       CreateAchRelationshipRequest const& request) {
    return broker_client_.post<BankRelationship>("/v1/accounts/" + account_id + "/ach_relationships",
                                                 to_json_payload(request));
}

void AlpacaClient::delete_ach_relationship(std::string const& account_id, std::string const& relationship_id) {
    broker_client_.del<void>("/v1/accounts/" + account_id + "/ach_relationships/" + relationship_id);
}

BankRelationshipsPage AlpacaClient::list_wire_relationships(std::string const& account_id) {
    return broker_client_.get<BankRelationshipsPage>("/v1/accounts/" + account_id + "/wire_relationships");
}

BankRelationship AlpacaClient::create_wire_relationship(std::string const& account_id,
                                                        CreateWireRelationshipRequest const& request) {
    return broker_client_.post<BankRelationship>("/v1/accounts/" + account_id + "/wire_relationships",
                                                 to_json_payload(request));
}

void AlpacaClient::delete_wire_relationship(std::string const& account_id, std::string const& relationship_id) {
    broker_client_.del<void>("/v1/accounts/" + account_id + "/wire_relationships/" + relationship_id);
}

} // namespace alpaca
