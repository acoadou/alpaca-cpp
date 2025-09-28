#include "alpaca/AlpacaClient.hpp"

#include <utility>

#include "alpaca/HttpClientFactory.hpp"

namespace alpaca {
AlpacaClient::AlpacaClient(Configuration config, HttpClientPtr http_client, RestClient::Options options)
  : config_(std::move(config)), http_client_(ensure_http_client(http_client)),
    trading_client_(config_, http_client_, options), market_data_client_(config_, http_client_, options),
    broker_client_(config_, http_client_, std::move(options)) {
}

AlpacaClient::AlpacaClient(Configuration config, RestClient::Options options)
  : AlpacaClient(std::move(config), nullptr, std::move(options)) {
}

AlpacaClient::AlpacaClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                           HttpClientPtr http_client, RestClient::Options options)
  : AlpacaClient(Configuration::FromEnvironment(environment, std::move(api_key_id), std::move(api_secret_key)),
                 std::move(http_client), std::move(options)) {
}

AlpacaClient::AlpacaClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                           RestClient::Options options)
  : AlpacaClient(environment, std::move(api_key_id), std::move(api_secret_key), nullptr, std::move(options)) {
}

TradingClient& AlpacaClient::trading() noexcept {
    return trading_client_;
}

TradingClient const& AlpacaClient::trading() const noexcept {
    return trading_client_;
}

MarketDataClient& AlpacaClient::market_data() noexcept {
    return market_data_client_;
}

MarketDataClient const& AlpacaClient::market_data() const noexcept {
    return market_data_client_;
}

BrokerClient& AlpacaClient::broker() noexcept {
    return broker_client_;
}

BrokerClient const& AlpacaClient::broker() const noexcept {
    return broker_client_;
}

Account AlpacaClient::get_account() {
    return trading().get_account();
}

AccountConfiguration AlpacaClient::get_account_configuration() {
    return trading().get_account_configuration();
}

AccountConfiguration AlpacaClient::update_account_configuration(AccountConfigurationUpdateRequest const& request) {
    return trading().update_account_configuration(request);
}

std::vector<Position> AlpacaClient::list_positions() {
    return trading().list_positions();
}

Position AlpacaClient::get_position(std::string const& symbol) {
    return trading().get_position(symbol);
}

Position AlpacaClient::close_position(std::string const& symbol, ClosePositionRequest const& request) {
    return trading().close_position(symbol, request);
}

std::vector<ClosePositionResponse> AlpacaClient::close_all_positions(CloseAllPositionsRequest const& request) {
    return trading().close_all_positions(request);
}

std::vector<OptionPosition> AlpacaClient::list_option_positions() {
    return trading().list_option_positions();
}

OptionPosition AlpacaClient::get_option_position(std::string const& symbol) {
    return trading().get_option_position(symbol);
}

OptionPosition AlpacaClient::close_option_position(std::string const& symbol,
                                                   CloseOptionPositionRequest const& request) {
    return trading().close_option_position(symbol, request);
}

void AlpacaClient::exercise_options_position(std::string const& symbol_or_contract_id) {
    trading().exercise_options_position(symbol_or_contract_id);
}

std::vector<Order> AlpacaClient::list_orders(ListOrdersRequest const& request) {
    return trading().list_orders(request);
}

Order AlpacaClient::get_order(std::string const& order_id) {
    return trading().get_order(order_id);
}

Order AlpacaClient::get_order_by_client_order_id(std::string const& client_order_id) {
    return trading().get_order_by_client_order_id(client_order_id);
}

void AlpacaClient::cancel_order(std::string const& order_id) {
    trading().cancel_order(order_id);
}

std::vector<CancelledOrderId> AlpacaClient::cancel_all_orders() {
    return trading().cancel_all_orders();
}

Order AlpacaClient::submit_order(NewOrderRequest const& request) {
    return trading().submit_order(request);
}

Order AlpacaClient::replace_order(std::string const& order_id, ReplaceOrderRequest const& request) {
    return trading().replace_order(order_id, request);
}

std::vector<OptionOrder> AlpacaClient::list_option_orders(ListOptionOrdersRequest const& request) {
    return trading().list_option_orders(request);
}

OptionOrder AlpacaClient::get_option_order(std::string const& order_id) {
    return trading().get_option_order(order_id);
}

OptionOrder AlpacaClient::get_option_order_by_client_order_id(std::string const& client_order_id) {
    return trading().get_option_order_by_client_order_id(client_order_id);
}

void AlpacaClient::cancel_option_order(std::string const& order_id) {
    trading().cancel_option_order(order_id);
}

std::vector<OptionCancelledOrderId> AlpacaClient::cancel_all_option_orders() {
    return trading().cancel_all_option_orders();
}

OptionOrder AlpacaClient::submit_option_order(NewOptionOrderRequest const& request) {
    return trading().submit_option_order(request);
}

OptionOrder AlpacaClient::replace_option_order(std::string const& order_id, ReplaceOptionOrderRequest const& request) {
    return trading().replace_option_order(order_id, request);
}

std::vector<CryptoOrder> AlpacaClient::list_crypto_orders(ListCryptoOrdersRequest request) {
    return trading().list_crypto_orders(std::move(request));
}

CryptoOrder AlpacaClient::get_crypto_order(std::string const& order_id) {
    return trading().get_crypto_order(order_id);
}

CryptoOrder AlpacaClient::get_crypto_order_by_client_order_id(std::string const& client_order_id) {
    return trading().get_crypto_order_by_client_order_id(client_order_id);
}

void AlpacaClient::cancel_crypto_order(std::string const& order_id) {
    trading().cancel_crypto_order(order_id);
}

std::vector<CryptoCancelledOrderId> AlpacaClient::cancel_all_crypto_orders() {
    return trading().cancel_all_crypto_orders();
}

CryptoOrder AlpacaClient::submit_crypto_order(NewCryptoOrderRequest const& request) {
    return trading().submit_crypto_order(request);
}

CryptoOrder AlpacaClient::replace_crypto_order(std::string const& order_id, ReplaceCryptoOrderRequest const& request) {
    return trading().replace_crypto_order(order_id, request);
}

std::vector<OtcOrder> AlpacaClient::list_otc_orders(ListOtcOrdersRequest request) {
    return trading().list_otc_orders(std::move(request));
}

OtcOrder AlpacaClient::get_otc_order(std::string const& order_id) {
    return trading().get_otc_order(order_id);
}

OtcOrder AlpacaClient::get_otc_order_by_client_order_id(std::string const& client_order_id) {
    return trading().get_otc_order_by_client_order_id(client_order_id);
}

void AlpacaClient::cancel_otc_order(std::string const& order_id) {
    trading().cancel_otc_order(order_id);
}

std::vector<OtcCancelledOrderId> AlpacaClient::cancel_all_otc_orders() {
    return trading().cancel_all_otc_orders();
}

OtcOrder AlpacaClient::submit_otc_order(NewOtcOrderRequest const& request) {
    return trading().submit_otc_order(request);
}

OtcOrder AlpacaClient::replace_otc_order(std::string const& order_id, ReplaceOtcOrderRequest const& request) {
    return trading().replace_otc_order(order_id, request);
}

OptionContractsResponse AlpacaClient::list_option_contracts(ListOptionContractsRequest const& request) {
    return trading().list_option_contracts(request);
}

OptionContract AlpacaClient::get_option_contract(std::string const& symbol) {
    return trading().get_option_contract(symbol);
}

OptionAnalyticsResponse AlpacaClient::list_option_analytics(ListOptionAnalyticsRequest const& request) {
    return trading().list_option_analytics(request);
}

OptionAnalytics AlpacaClient::get_option_analytics(std::string const& symbol) {
    return trading().get_option_analytics(symbol);
}

Clock AlpacaClient::get_clock() {
    return trading().get_clock();
}

std::vector<CalendarDay> AlpacaClient::get_calendar(CalendarRequest const& request) {
    return trading().get_calendar(request);
}

std::vector<Asset> AlpacaClient::list_assets(ListAssetsRequest const& request) {
    return trading().list_assets(request);
}

Asset AlpacaClient::get_asset(std::string const& symbol) {
    return trading().get_asset(symbol);
}

std::vector<AccountActivity> AlpacaClient::get_account_activities(AccountActivitiesRequest const& request) {
    return trading().get_account_activities(request);
}

PortfolioHistory AlpacaClient::get_portfolio_history(PortfolioHistoryRequest const& request) {
    return trading().get_portfolio_history(request);
}

std::vector<Watchlist> AlpacaClient::list_watchlists() {
    return trading().list_watchlists();
}

Watchlist AlpacaClient::get_watchlist(std::string const& id) {
    return trading().get_watchlist(id);
}

Watchlist AlpacaClient::get_watchlist_by_name(std::string const& name) {
    return trading().get_watchlist_by_name(name);
}

Watchlist AlpacaClient::create_watchlist(CreateWatchlistRequest const& request) {
    return trading().create_watchlist(request);
}

Watchlist AlpacaClient::update_watchlist(std::string const& id, UpdateWatchlistRequest const& request) {
    return trading().update_watchlist(id, request);
}

Watchlist AlpacaClient::add_asset_to_watchlist(std::string const& id, std::string const& symbol) {
    return trading().add_asset_to_watchlist(id, symbol);
}

Watchlist AlpacaClient::remove_asset_from_watchlist(std::string const& id, std::string const& symbol) {
    return trading().remove_asset_from_watchlist(id, symbol);
}

void AlpacaClient::delete_watchlist(std::string const& id) {
    trading().delete_watchlist(id);
}

LatestStockTrade AlpacaClient::get_latest_stock_trade(std::string const& symbol) {
    return market_data().get_latest_stock_trade(symbol);
}

LatestStockQuote AlpacaClient::get_latest_stock_quote(std::string const& symbol) {
    return market_data().get_latest_stock_quote(symbol);
}

LatestStockTrades AlpacaClient::get_latest_stock_trades(LatestStocksRequest const& request) {
    return market_data().get_latest_stock_trades(request);
}

LatestStockQuotes AlpacaClient::get_latest_stock_quotes(LatestStocksRequest const& request) {
    return market_data().get_latest_stock_quotes(request);
}

LatestStockBars AlpacaClient::get_latest_stock_bars(LatestStocksRequest const& request) {
    return market_data().get_latest_stock_bars(request);
}

LatestOptionTrades AlpacaClient::get_latest_option_trades(LatestOptionsRequest const& request) {
    return market_data().get_latest_option_trades(request);
}

LatestOptionQuotes AlpacaClient::get_latest_option_quotes(LatestOptionsRequest const& request) {
    return market_data().get_latest_option_quotes(request);
}

LatestOptionBars AlpacaClient::get_latest_option_bars(LatestOptionsRequest const& request) {
    return market_data().get_latest_option_bars(request);
}

LatestCryptoTrades AlpacaClient::get_latest_crypto_trades(std::string const& feed, LatestCryptoRequest const& request) {
    return market_data().get_latest_crypto_trades(feed, request);
}

LatestCryptoQuotes AlpacaClient::get_latest_crypto_quotes(std::string const& feed, LatestCryptoRequest const& request) {
    return market_data().get_latest_crypto_quotes(feed, request);
}

LatestCryptoBars AlpacaClient::get_latest_crypto_bars(std::string const& feed, LatestCryptoRequest const& request) {
    return market_data().get_latest_crypto_bars(feed, request);
}

MultiStockOrderbooks AlpacaClient::get_stock_orderbooks(LatestStockOrderbooksRequest const& request) {
    return market_data().get_stock_orderbooks(request);
}

MultiOptionOrderbooks AlpacaClient::get_option_orderbooks(LatestOptionOrderbooksRequest const& request) {
    return market_data().get_option_orderbooks(request);
}

MultiCryptoOrderbooks AlpacaClient::get_crypto_orderbooks(std::string const& feed,
                                                          LatestCryptoOrderbooksRequest const& request) {
    return market_data().get_crypto_orderbooks(feed, request);
}

StockBars AlpacaClient::get_stock_bars(std::string const& symbol, StockBarsRequest const& request) {
    return market_data().get_stock_bars(symbol, request);
}

std::vector<StockBar> AlpacaClient::get_all_stock_bars(std::string const& symbol, StockBarsRequest request) {
    return market_data().get_all_stock_bars(symbol, std::move(request));
}

PaginatedVectorRange<StockBarsRequest, StockBars, StockBar>
AlpacaClient::stock_bars_range(std::string const& symbol, StockBarsRequest request) const {
    return market_data().stock_bars_range(symbol, std::move(request));
}

StockSnapshot AlpacaClient::get_stock_snapshot(std::string const& symbol) {
    return market_data().get_stock_snapshot(symbol);
}

NewsResponse AlpacaClient::get_news(NewsRequest const& request) {
    return market_data().get_news(request);
}

PaginatedVectorRange<NewsRequest, NewsResponse, NewsArticle> AlpacaClient::news_range(NewsRequest request) const {
    return market_data().news_range(std::move(request));
}

HistoricalAuctionsResponse AlpacaClient::get_stock_auctions(std::string const& symbol,
                                                            HistoricalAuctionsRequest const& request) {
    return market_data().get_stock_auctions(symbol, request);
}

HistoricalAuctionsResponse AlpacaClient::get_auctions(HistoricalAuctionsRequest const& request) {
    return market_data().get_auctions(request);
}

PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>
AlpacaClient::stock_auctions_range(std::string const& symbol, HistoricalAuctionsRequest request) const {
    return market_data().stock_auctions_range(symbol, std::move(request));
}

PaginatedVectorRange<HistoricalAuctionsRequest, HistoricalAuctionsResponse, StockAuction>
AlpacaClient::auctions_range(HistoricalAuctionsRequest request) const {
    return market_data().auctions_range(std::move(request));
}

CorporateActionAnnouncementsResponse
AlpacaClient::get_corporate_announcements(CorporateActionAnnouncementsRequest const& request) {
    return market_data().get_corporate_announcements(request);
}

CorporateActionEventsResponse AlpacaClient::get_corporate_actions(CorporateActionEventsRequest const& request) {
    return market_data().get_corporate_actions(request);
}

MultiStockBars AlpacaClient::get_stock_aggregates(MultiStockBarsRequest const& request) {
    return market_data().get_stock_aggregates(request);
}

MultiStockQuotes AlpacaClient::get_stock_quotes(MultiStockQuotesRequest const& request) {
    return market_data().get_stock_quotes(request);
}

MultiStockTrades AlpacaClient::get_stock_trades(MultiStockTradesRequest const& request) {
    return market_data().get_stock_trades(request);
}

MultiOptionBars AlpacaClient::get_option_aggregates(MultiOptionBarsRequest const& request) {
    return market_data().get_option_aggregates(request);
}

MultiOptionQuotes AlpacaClient::get_option_quotes(MultiOptionQuotesRequest const& request) {
    return market_data().get_option_quotes(request);
}

MultiOptionTrades AlpacaClient::get_option_trades(MultiOptionTradesRequest const& request) {
    return market_data().get_option_trades(request);
}

MultiCryptoBars AlpacaClient::get_crypto_aggregates(MultiCryptoBarsRequest const& request) {
    return market_data().get_crypto_aggregates(request);
}

MultiCryptoQuotes AlpacaClient::get_crypto_quotes(MultiCryptoQuotesRequest const& request) {
    return market_data().get_crypto_quotes(request);
}

MultiCryptoTrades AlpacaClient::get_crypto_trades(MultiCryptoTradesRequest const& request) {
    return market_data().get_crypto_trades(request);
}

LatestCryptoTrades AlpacaClient::get_latest_crypto_trade(std::string const& feed,
                                                         LatestCryptoDataRequest const& request) {
    return market_data().get_latest_crypto_trade(feed, request);
}

LatestCryptoQuotes AlpacaClient::get_latest_crypto_quote(std::string const& feed,
                                                         LatestCryptoDataRequest const& request) {
    return market_data().get_latest_crypto_quote(feed, request);
}

LatestCryptoBars AlpacaClient::get_latest_crypto_bar(std::string const& feed, LatestCryptoDataRequest const& request) {
    return market_data().get_latest_crypto_bar(feed, request);
}

LatestCryptoOrderbooks AlpacaClient::get_latest_crypto_orderbook(std::string const& feed,
                                                                 LatestCryptoOrderbookRequest const& request) {
    return market_data().get_latest_crypto_orderbook(feed, request);
}

ListExchangesResponse AlpacaClient::list_exchanges(ListExchangesRequest const& request) {
    return market_data().list_exchanges(request);
}

ListTradeConditionsResponse AlpacaClient::list_trade_conditions(ListTradeConditionsRequest const& request) {
    return market_data().list_trade_conditions(request);
}

MarketMoversResponse AlpacaClient::get_top_market_movers(MarketMoversRequest const& request) {
    return market_data().get_top_market_movers(request);
}

MostActiveStocksResponse AlpacaClient::get_most_active_stocks(MostActiveStocksRequest const& request) {
    return market_data().get_most_active_stocks(request);
}

BrokerAccountsPage AlpacaClient::list_broker_accounts(ListBrokerAccountsRequest const& request) {
    return broker().list_accounts(request);
}

PaginatedVectorRange<ListBrokerAccountsRequest, BrokerAccountsPage, BrokerAccount>
AlpacaClient::list_broker_accounts_range(ListBrokerAccountsRequest request) const {
    return broker().list_accounts_range(std::move(request));
}

BrokerAccount AlpacaClient::get_broker_account(std::string const& account_id) {
    return broker().get_account(account_id);
}

BrokerAccount AlpacaClient::create_broker_account(CreateBrokerAccountRequest const& request) {
    return broker().create_account(request);
}

BrokerAccount AlpacaClient::update_broker_account(std::string const& account_id,
                                                  UpdateBrokerAccountRequest const& request) {
    return broker().update_account(account_id, request);
}

void AlpacaClient::delete_broker_account(std::string const& account_id) {
    broker().close_account(account_id);
}

std::vector<AccountDocument> AlpacaClient::list_account_documents(std::string const& account_id) {
    return broker().list_documents(account_id);
}

AccountDocument AlpacaClient::upload_account_document(std::string const& account_id,
                                                      CreateAccountDocumentRequest const& request) {
    return broker().upload_document(account_id, request);
}

TransfersPage AlpacaClient::list_account_transfers(std::string const& account_id, ListTransfersRequest const& request) {
    return broker().list_transfers(account_id, request);
}

Transfer AlpacaClient::create_account_transfer(std::string const& account_id, CreateTransferRequest const& request) {
    return broker().create_transfer(account_id, request);
}

Transfer AlpacaClient::get_transfer(std::string const& transfer_id) {
    return broker().get_transfer(transfer_id);
}

void AlpacaClient::cancel_transfer(std::string const& account_id, std::string const& transfer_id) {
    broker().cancel_transfer(account_id, transfer_id);
}

PaginatedVectorRange<ListTransfersRequest, TransfersPage, Transfer>
AlpacaClient::list_account_transfers_range(std::string const& account_id, ListTransfersRequest request) const {
    return broker().list_transfers_range(account_id, std::move(request));
}

JournalsPage AlpacaClient::list_journals(ListJournalsRequest const& request) {
    return broker().list_journals(request);
}

Journal AlpacaClient::create_journal(CreateJournalRequest const& request) {
    return broker().create_journal(request);
}

Journal AlpacaClient::get_journal(std::string const& journal_id) {
    return broker().get_journal(journal_id);
}

void AlpacaClient::cancel_journal(std::string const& journal_id) {
    broker().cancel_journal(journal_id);
}

PaginatedVectorRange<ListJournalsRequest, JournalsPage, Journal>
AlpacaClient::list_journals_range(ListJournalsRequest request) const {
    return broker().list_journals_range(std::move(request));
}

BankRelationshipsPage AlpacaClient::list_ach_relationships(std::string const& account_id) {
    return broker().list_ach_relationships(account_id);
}

BankRelationship AlpacaClient::create_ach_relationship(std::string const& account_id,
                                                       CreateAchRelationshipRequest const& request) {
    return broker().create_ach_relationship(account_id, request);
}

void AlpacaClient::delete_ach_relationship(std::string const& account_id, std::string const& relationship_id) {
    broker().delete_ach_relationship(account_id, relationship_id);
}

BankRelationshipsPage AlpacaClient::list_wire_relationships(std::string const& account_id) {
    return broker().list_wire_relationships(account_id);
}

BankRelationship AlpacaClient::create_wire_relationship(std::string const& account_id,
                                                        CreateWireRelationshipRequest const& request) {
    return broker().create_wire_relationship(account_id, request);
}

void AlpacaClient::delete_wire_relationship(std::string const& account_id, std::string const& relationship_id) {
    broker().delete_wire_relationship(account_id, relationship_id);
}

std::vector<BrokerWatchlist> AlpacaClient::list_broker_watchlists(std::string const& account_id) {
    return broker().list_watchlists(account_id);
}

BrokerWatchlist AlpacaClient::get_broker_watchlist(std::string const& account_id, std::string const& watchlist_id) {
    return broker().get_watchlist(account_id, watchlist_id);
}

BrokerWatchlist AlpacaClient::create_broker_watchlist(std::string const& account_id,
                                                      CreateBrokerWatchlistRequest const& request) {
    return broker().create_watchlist(account_id, request);
}

BrokerWatchlist AlpacaClient::update_broker_watchlist(std::string const& account_id, std::string const& watchlist_id,
                                                      UpdateBrokerWatchlistRequest const& request) {
    return broker().update_watchlist(account_id, watchlist_id, request);
}

BrokerWatchlist AlpacaClient::add_asset_to_broker_watchlist(std::string const& account_id,
                                                            std::string const& watchlist_id,
                                                            std::string const& symbol) {
    return broker().add_asset_to_watchlist(account_id, watchlist_id, symbol);
}

BrokerWatchlist AlpacaClient::remove_asset_from_broker_watchlist(std::string const& account_id,
                                                                 std::string const& watchlist_id,
                                                                 std::string const& symbol) {
    return broker().remove_asset_from_watchlist(account_id, watchlist_id, symbol);
}

void AlpacaClient::delete_broker_watchlist(std::string const& account_id, std::string const& watchlist_id) {
    broker().delete_watchlist(account_id, watchlist_id);
}

std::vector<RebalancingPortfolio>
AlpacaClient::list_rebalancing_portfolios(ListRebalancingPortfoliosRequest const& request) {
    return broker().list_rebalancing_portfolios(request);
}

RebalancingPortfolio AlpacaClient::get_rebalancing_portfolio(std::string const& portfolio_id) {
    return broker().get_rebalancing_portfolio(portfolio_id);
}

RebalancingPortfolio AlpacaClient::create_rebalancing_portfolio(CreateRebalancingPortfolioRequest const& request) {
    return broker().create_rebalancing_portfolio(request);
}

RebalancingPortfolio AlpacaClient::update_rebalancing_portfolio(std::string const& portfolio_id,
                                                                UpdateRebalancingPortfolioRequest const& request) {
    return broker().update_rebalancing_portfolio(portfolio_id, request);
}

void AlpacaClient::deactivate_rebalancing_portfolio(std::string const& portfolio_id) {
    broker().deactivate_rebalancing_portfolio(portfolio_id);
}

RebalancingSubscriptionsPage
AlpacaClient::list_rebalancing_subscriptions(ListRebalancingSubscriptionsRequest const& request) {
    return broker().list_rebalancing_subscriptions(request);
}

PaginatedVectorRange<ListRebalancingSubscriptionsRequest, RebalancingSubscriptionsPage, RebalancingSubscription>
AlpacaClient::list_rebalancing_subscriptions_range(ListRebalancingSubscriptionsRequest request) const {
    return broker().list_rebalancing_subscriptions_range(std::move(request));
}

RebalancingSubscription AlpacaClient::get_rebalancing_subscription(std::string const& subscription_id) {
    return broker().get_rebalancing_subscription(subscription_id);
}

RebalancingSubscription
AlpacaClient::create_rebalancing_subscription(CreateRebalancingSubscriptionRequest const& request) {
    return broker().create_rebalancing_subscription(request);
}

ManagedPortfolioHistory AlpacaClient::get_managed_portfolio_history(std::string const& account_id,
                                                                    ManagedPortfolioHistoryRequest const& request) {
    return broker().get_managed_portfolio_history(account_id, request);
}

} // namespace alpaca
