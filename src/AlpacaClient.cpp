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

std::string make_data_beta_base_url(const std::string& data_base_url) {
  constexpr std::string_view v2_suffix{"/v2"};
  if (data_base_url.ends_with(v2_suffix)) {
    return data_base_url.substr(0, data_base_url.size() - v2_suffix.size()) + "/v1beta1";
  }
  if (data_base_url.ends_with('/')) {
    return data_base_url + "v1beta1";
  }
  return data_base_url + "/v1beta1";
}

Json to_json_payload(const AccountConfigurationUpdateRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const NewOrderRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const ReplaceOrderRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateWatchlistRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const UpdateWatchlistRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateBrokerAccountRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const UpdateBrokerAccountRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateAccountDocumentRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateTransferRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateJournalRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateAchRelationshipRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json to_json_payload(const CreateWireRelationshipRequest& request) {
  Json payload;
  to_json(payload, request);
  return payload;
}

Json symbol_payload(const std::string& symbol) {
  return Json{{"symbol", symbol}};
}
}  // namespace

AlpacaClient::AlpacaClient(Configuration config, HttpClientPtr http_client)
    : trading_client_(config, ensure_http_client(http_client), config.trading_base_url),
      data_client_(config, ensure_http_client(http_client), config.data_base_url),
      data_beta_client_(config, ensure_http_client(http_client),
                        make_data_beta_base_url(config.data_base_url)),
      broker_client_(config, ensure_http_client(http_client), config.broker_base_url) {}

Account AlpacaClient::get_account() { return trading_client_.get<Account>("/v2/account"); }

AccountConfiguration AlpacaClient::get_account_configuration() {
  return trading_client_.get<AccountConfiguration>("/v2/account/configurations");
}

AccountConfiguration AlpacaClient::update_account_configuration(const AccountConfigurationUpdateRequest& request) {
  return trading_client_.patch<AccountConfiguration>("/v2/account/configurations", to_json_payload(request));
}

std::vector<Position> AlpacaClient::list_positions() {
  return trading_client_.get<std::vector<Position>>("/v2/positions");
}

Position AlpacaClient::get_position(const std::string& symbol) {
  return trading_client_.get<Position>("/v2/positions/" + symbol);
}

Position AlpacaClient::close_position(const std::string& symbol, const ClosePositionRequest& request) {
  return trading_client_.del<Position>("/v2/positions/" + symbol, request.to_query_params());
}

std::vector<Order> AlpacaClient::list_orders(const ListOrdersRequest& request) {
  return trading_client_.get<std::vector<Order>>("/v2/orders", request.to_query_params());
}

Order AlpacaClient::get_order(const std::string& order_id) {
  return trading_client_.get<Order>("/v2/orders/" + order_id);
}

Order AlpacaClient::get_order_by_client_order_id(const std::string& client_order_id) {
  return trading_client_.get<Order>("/v2/orders:by_client_order_id", {{"client_order_id", client_order_id}});
}

void AlpacaClient::cancel_order(const std::string& order_id) {
  trading_client_.del<void>("/v2/orders/" + order_id);
}

std::vector<CancelledOrderId> AlpacaClient::cancel_all_orders() {
  return trading_client_.del<std::vector<CancelledOrderId>>("/v2/orders");
}

Order AlpacaClient::submit_order(const NewOrderRequest& request) {
  return trading_client_.post<Order>("/v2/orders", to_json_payload(request));
}

Order AlpacaClient::replace_order(const std::string& order_id, const ReplaceOrderRequest& request) {
  return trading_client_.patch<Order>("/v2/orders/" + order_id, to_json_payload(request));
}

Clock AlpacaClient::get_clock() { return trading_client_.get<Clock>("/v2/clock"); }

std::vector<CalendarDay> AlpacaClient::get_calendar(const CalendarRequest& request) {
  return trading_client_.get<std::vector<CalendarDay>>("/v2/calendar", request.to_query_params());
}

std::vector<Asset> AlpacaClient::list_assets(const ListAssetsRequest& request) {
  return trading_client_.get<std::vector<Asset>>("/v2/assets", request.to_query_params());
}

Asset AlpacaClient::get_asset(const std::string& symbol) {
  return trading_client_.get<Asset>("/v2/assets/" + symbol);
}

std::vector<AccountActivity> AlpacaClient::get_account_activities(const AccountActivitiesRequest& request) {
  return trading_client_.get<std::vector<AccountActivity>>("/v2/account/activities", request.to_query_params());
}

PortfolioHistory AlpacaClient::get_portfolio_history(const PortfolioHistoryRequest& request) {
  return trading_client_.get<PortfolioHistory>("/v2/account/portfolio/history", request.to_query_params());
}

std::vector<Watchlist> AlpacaClient::list_watchlists() {
  return trading_client_.get<std::vector<Watchlist>>("/v2/watchlists");
}

Watchlist AlpacaClient::get_watchlist(const std::string& id) {
  return trading_client_.get<Watchlist>("/v2/watchlists/" + id);
}

Watchlist AlpacaClient::get_watchlist_by_name(const std::string& name) {
  return trading_client_.get<Watchlist>("/v2/watchlists:by_name", {{"name", name}});
}

Watchlist AlpacaClient::create_watchlist(const CreateWatchlistRequest& request) {
  return trading_client_.post<Watchlist>("/v2/watchlists", to_json_payload(request));
}

Watchlist AlpacaClient::update_watchlist(const std::string& id, const UpdateWatchlistRequest& request) {
  return trading_client_.put<Watchlist>("/v2/watchlists/" + id, to_json_payload(request));
}

Watchlist AlpacaClient::add_asset_to_watchlist(const std::string& id, const std::string& symbol) {
  return trading_client_.post<Watchlist>("/v2/watchlists/" + id, symbol_payload(symbol));
}

Watchlist AlpacaClient::remove_asset_from_watchlist(const std::string& id, const std::string& symbol) {
  return trading_client_.del<Watchlist>("/v2/watchlists/" + id + "/" + symbol);
}

void AlpacaClient::delete_watchlist(const std::string& id) {
  trading_client_.del<void>("/v2/watchlists/" + id);
}

LatestStockTrade AlpacaClient::get_latest_stock_trade(const std::string& symbol) {
  return data_client_.get<LatestStockTrade>("stocks/" + symbol + "/trades/latest");
}

LatestStockQuote AlpacaClient::get_latest_stock_quote(const std::string& symbol) {
  return data_client_.get<LatestStockQuote>("stocks/" + symbol + "/quotes/latest");
}

StockBars AlpacaClient::get_stock_bars(const std::string& symbol, const StockBarsRequest& request) {
  return data_client_.get<StockBars>("stocks/" + symbol + "/bars", request.to_query_params());
}

std::vector<StockBar> AlpacaClient::get_all_stock_bars(const std::string& symbol, StockBarsRequest request) {
  std::vector<StockBar> all_bars;
  std::optional<std::string> next_page = request.page_token;

  while (true) {
    if (next_page.has_value()) {
      request.page_token = next_page;
    } else {
      request.page_token.reset();
    }

    const auto page = get_stock_bars(symbol, request);
    all_bars.insert(all_bars.end(), page.bars.begin(), page.bars.end());

    next_page = page.next_page_token;
    if (!next_page.has_value()) {
      break;
    }
  }

  return all_bars;
}

StockSnapshot AlpacaClient::get_stock_snapshot(const std::string& symbol) {
  return data_client_.get<StockSnapshot>("stocks/" + symbol + "/snapshot");
}

NewsResponse AlpacaClient::get_news(const QueryParams& params) {
  return data_beta_client_.get<NewsResponse>("news", params);
}

CorporateActionAnnouncementsResponse AlpacaClient::get_corporate_announcements(const QueryParams& params) {
  return data_beta_client_.get<CorporateActionAnnouncementsResponse>("corporate-actions/announcements", params);
}

CorporateActionEventsResponse AlpacaClient::get_corporate_actions(const QueryParams& params) {
  return data_beta_client_.get<CorporateActionEventsResponse>("corporate-actions/events", params);
}

MultiStockBars AlpacaClient::get_stock_aggregates(const QueryParams& params) {
  return data_client_.get<MultiStockBars>("stocks/bars", params);
}

MultiStockQuotes AlpacaClient::get_stock_quotes(const QueryParams& params) {
  return data_client_.get<MultiStockQuotes>("stocks/quotes", params);
}

MultiStockTrades AlpacaClient::get_stock_trades(const QueryParams& params) {
  return data_client_.get<MultiStockTrades>("stocks/trades", params);
}

MultiOptionBars AlpacaClient::get_option_aggregates(const QueryParams& params) {
  return data_beta_client_.get<MultiOptionBars>("options/bars", params);
}

MultiOptionQuotes AlpacaClient::get_option_quotes(const QueryParams& params) {
  return data_beta_client_.get<MultiOptionQuotes>("options/quotes", params);
}

MultiOptionTrades AlpacaClient::get_option_trades(const QueryParams& params) {
  return data_beta_client_.get<MultiOptionTrades>("options/trades", params);
}

MultiCryptoBars AlpacaClient::get_crypto_aggregates(const QueryParams& params) {
  return data_beta_client_.get<MultiCryptoBars>("crypto/bars", params);
}

MultiCryptoQuotes AlpacaClient::get_crypto_quotes(const QueryParams& params) {
  return data_beta_client_.get<MultiCryptoQuotes>("crypto/quotes", params);
}

MultiCryptoTrades AlpacaClient::get_crypto_trades(const QueryParams& params) {
  return data_beta_client_.get<MultiCryptoTrades>("crypto/trades", params);
}

BrokerAccountsPage AlpacaClient::list_broker_accounts(const ListBrokerAccountsRequest& request) {
  return broker_client_.get<BrokerAccountsPage>("/v1/accounts", request.to_query_params());
}

BrokerAccount AlpacaClient::get_broker_account(const std::string& account_id) {
  return broker_client_.get<BrokerAccount>("/v1/accounts/" + account_id);
}

BrokerAccount AlpacaClient::create_broker_account(const CreateBrokerAccountRequest& request) {
  return broker_client_.post<BrokerAccount>("/v1/accounts", to_json_payload(request));
}

BrokerAccount AlpacaClient::update_broker_account(const std::string& account_id,
                                                  const UpdateBrokerAccountRequest& request) {
  return broker_client_.patch<BrokerAccount>("/v1/accounts/" + account_id, to_json_payload(request));
}

void AlpacaClient::delete_broker_account(const std::string& account_id) {
  broker_client_.post<void>("/v1/accounts/" + account_id + "/actions/close", Json::object());
}

std::vector<AccountDocument> AlpacaClient::list_account_documents(const std::string& account_id) {
  return broker_client_.get<std::vector<AccountDocument>>("/v1/accounts/" + account_id + "/documents");
}

AccountDocument AlpacaClient::upload_account_document(const std::string& account_id,
                                                      const CreateAccountDocumentRequest& request) {
  return broker_client_.post<AccountDocument>("/v1/accounts/" + account_id + "/documents",
                                              to_json_payload(request));
}

TransfersPage AlpacaClient::list_account_transfers(const std::string& account_id,
                                                   const ListTransfersRequest& request) {
  return broker_client_.get<TransfersPage>("/v1/accounts/" + account_id + "/transfers",
                                           request.to_query_params());
}

Transfer AlpacaClient::create_account_transfer(const std::string& account_id,
                                               const CreateTransferRequest& request) {
  return broker_client_.post<Transfer>("/v1/accounts/" + account_id + "/transfers",
                                       to_json_payload(request));
}

Transfer AlpacaClient::get_transfer(const std::string& transfer_id) {
  return broker_client_.get<Transfer>("/v1/transfers/" + transfer_id);
}

void AlpacaClient::cancel_transfer(const std::string& account_id, const std::string& transfer_id) {
  broker_client_.del<void>("/v1/accounts/" + account_id + "/transfers/" + transfer_id);
}

JournalsPage AlpacaClient::list_journals(const ListJournalsRequest& request) {
  return broker_client_.get<JournalsPage>("/v1/journals", request.to_query_params());
}

Journal AlpacaClient::create_journal(const CreateJournalRequest& request) {
  return broker_client_.post<Journal>("/v1/journals", to_json_payload(request));
}

Journal AlpacaClient::get_journal(const std::string& journal_id) {
  return broker_client_.get<Journal>("/v1/journals/" + journal_id);
}

void AlpacaClient::cancel_journal(const std::string& journal_id) {
  broker_client_.del<void>("/v1/journals/" + journal_id);
}

BankRelationshipsPage AlpacaClient::list_ach_relationships(const std::string& account_id) {
  return broker_client_.get<BankRelationshipsPage>("/v1/accounts/" + account_id + "/ach_relationships");
}

BankRelationship AlpacaClient::create_ach_relationship(const std::string& account_id,
                                                        const CreateAchRelationshipRequest& request) {
  return broker_client_.post<BankRelationship>("/v1/accounts/" + account_id + "/ach_relationships",
                                               to_json_payload(request));
}

void AlpacaClient::delete_ach_relationship(const std::string& account_id, const std::string& relationship_id) {
  broker_client_.del<void>("/v1/accounts/" + account_id + "/ach_relationships/" + relationship_id);
}

BankRelationshipsPage AlpacaClient::list_wire_relationships(const std::string& account_id) {
  return broker_client_.get<BankRelationshipsPage>("/v1/accounts/" + account_id + "/wire_relationships");
}

BankRelationship AlpacaClient::create_wire_relationship(const std::string& account_id,
                                                         const CreateWireRelationshipRequest& request) {
  return broker_client_.post<BankRelationship>("/v1/accounts/" + account_id + "/wire_relationships",
                                               to_json_payload(request));
}

void AlpacaClient::delete_wire_relationship(const std::string& account_id, const std::string& relationship_id) {
  broker_client_.del<void>("/v1/accounts/" + account_id + "/wire_relationships/" + relationship_id);
}

}  // namespace alpaca
