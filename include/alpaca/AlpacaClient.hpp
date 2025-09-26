#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/Configuration.hpp"
#include "alpaca/models/Account.hpp"
#include "alpaca/models/AccountActivity.hpp"
#include "alpaca/models/AccountConfiguration.hpp"
#include "alpaca/models/Asset.hpp"
#include "alpaca/models/CalendarDay.hpp"
#include "alpaca/models/Clock.hpp"
#include "alpaca/models/CorporateActions.hpp"
#include "alpaca/models/Broker.hpp"
#include "alpaca/models/News.hpp"
#include "alpaca/models/PortfolioHistory.hpp"
#include "alpaca/models/MarketData.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/Position.hpp"
#include "alpaca/models/Watchlist.hpp"
#include "alpaca/RestClient.hpp"

namespace alpaca {

/// High level client that exposes strongly typed wrappers around Alpaca REST endpoints.
class AlpacaClient {
 public:
  explicit AlpacaClient(Configuration config, HttpClientPtr http_client = nullptr);

  /// Returns account information for the authenticated user.
  [[nodiscard]] Account get_account();

  /// Returns the account configuration for the authenticated user.
  [[nodiscard]] AccountConfiguration get_account_configuration();

  /// Updates account configuration fields using a partial request.
  [[nodiscard]] AccountConfiguration update_account_configuration(
      const AccountConfigurationUpdateRequest& request);

  /// Returns positions currently held in the authenticated account.
  [[nodiscard]] std::vector<Position> list_positions();

  /// Returns a single open position by symbol.
  [[nodiscard]] Position get_position(const std::string& symbol);

  /// Closes the position identified by \p symbol with optional close parameters.
  [[nodiscard]] Position close_position(const std::string& symbol, const ClosePositionRequest& request = {});

  /// Returns all orders that match the supplied filters.
  [[nodiscard]] std::vector<Order> list_orders(const ListOrdersRequest& request = {});

  /// Returns details for a single order.
  [[nodiscard]] Order get_order(const std::string& order_id);

  /// Returns an order using the provided client order identifier.
  [[nodiscard]] Order get_order_by_client_order_id(const std::string& client_order_id);

  /// Cancels the order identified by \p order_id.
  void cancel_order(const std::string& order_id);

  /// Cancels all open orders and returns the identifiers that were cancelled.
  [[nodiscard]] std::vector<CancelledOrderId> cancel_all_orders();

  /// Submits a new order and returns the created order representation.
  [[nodiscard]] Order submit_order(const NewOrderRequest& request);

  /// Replaces an existing order using the Alpaca replace endpoint.
  [[nodiscard]] Order replace_order(const std::string& order_id, const ReplaceOrderRequest& request);

  /// Returns the current trading clock.
  [[nodiscard]] Clock get_clock();

  /// Returns the trading calendar for the provided filters.
  [[nodiscard]] std::vector<CalendarDay> get_calendar(const CalendarRequest& request = {});

  /// Returns a list of tradable assets.
  [[nodiscard]] std::vector<Asset> list_assets(const ListAssetsRequest& request = {});

  /// Returns a single asset by symbol.
  [[nodiscard]] Asset get_asset(const std::string& symbol);

  /// Returns account activities matching the optional filters.
  [[nodiscard]] std::vector<AccountActivity> get_account_activities(
      const AccountActivitiesRequest& request = {});

  /// Returns the account's portfolio history for the optional request.
  [[nodiscard]] PortfolioHistory get_portfolio_history(const PortfolioHistoryRequest& request = {});

  /// Returns all watchlists available for the account.
  [[nodiscard]] std::vector<Watchlist> list_watchlists();

  /// Returns a single watchlist by identifier.
  [[nodiscard]] Watchlist get_watchlist(const std::string& id);

  /// Returns a single watchlist by name.
  [[nodiscard]] Watchlist get_watchlist_by_name(const std::string& name);

  /// Creates a new watchlist.
  [[nodiscard]] Watchlist create_watchlist(const CreateWatchlistRequest& request);

  /// Updates an existing watchlist.
  [[nodiscard]] Watchlist update_watchlist(const std::string& id, const UpdateWatchlistRequest& request);

  /// Adds an asset to the watchlist.
  [[nodiscard]] Watchlist add_asset_to_watchlist(const std::string& id, const std::string& symbol);

  /// Removes an asset from the watchlist.
  [[nodiscard]] Watchlist remove_asset_from_watchlist(const std::string& id, const std::string& symbol);

  /// Deletes an existing watchlist.
  void delete_watchlist(const std::string& id);

  /// Returns the latest reported trade for the supplied stock symbol.
  [[nodiscard]] LatestStockTrade get_latest_stock_trade(const std::string& symbol);

  /// Returns the latest NBBO quote for the supplied stock symbol.
  [[nodiscard]] LatestStockQuote get_latest_stock_quote(const std::string& symbol);

  /// Returns historical bars for the supplied stock symbol using optional filters.
  [[nodiscard]] StockBars get_stock_bars(const std::string& symbol, const StockBarsRequest& request = {});

  /// Returns all available historical bars for the supplied stock symbol, automatically traversing pagination.
  [[nodiscard]] std::vector<StockBar> get_all_stock_bars(const std::string& symbol, StockBarsRequest request = {});

  /// Returns a consolidated market data snapshot for the supplied stock symbol.
  [[nodiscard]] StockSnapshot get_stock_snapshot(const std::string& symbol);

  /// Retrieves Alpaca news articles using the optional filters supplied in \p params.
  [[nodiscard]] NewsResponse get_news(const QueryParams& params = {});

  /// Retrieves corporate action announcements with optional filters.
  [[nodiscard]] CorporateActionAnnouncementsResponse get_corporate_announcements(const QueryParams& params = {});

  /// Retrieves corporate action events with optional filters.
  [[nodiscard]] CorporateActionEventsResponse get_corporate_actions(const QueryParams& params = {});

  /// Retrieves multi-symbol stock aggregates from the Alpaca market data API.
  [[nodiscard]] MultiStockBars get_stock_aggregates(const QueryParams& params = {});

  /// Retrieves multi-symbol stock quotes from the Alpaca market data API.
  [[nodiscard]] MultiStockQuotes get_stock_quotes(const QueryParams& params = {});

  /// Retrieves multi-symbol stock trades from the Alpaca market data API.
  [[nodiscard]] MultiStockTrades get_stock_trades(const QueryParams& params = {});

  /// Retrieves options aggregates across one or more contracts.
  [[nodiscard]] MultiOptionBars get_option_aggregates(const QueryParams& params = {});

  /// Retrieves the latest options quotes across one or more contracts.
  [[nodiscard]] MultiOptionQuotes get_option_quotes(const QueryParams& params = {});

  /// Retrieves the latest options trades across one or more contracts.
  [[nodiscard]] MultiOptionTrades get_option_trades(const QueryParams& params = {});

  /// Retrieves crypto aggregates across one or more symbols.
  [[nodiscard]] MultiCryptoBars get_crypto_aggregates(const QueryParams& params = {});

  /// Retrieves crypto quotes across one or more symbols.
  [[nodiscard]] MultiCryptoQuotes get_crypto_quotes(const QueryParams& params = {});

  /// Retrieves crypto trades across one or more symbols.
  [[nodiscard]] MultiCryptoTrades get_crypto_trades(const QueryParams& params = {});

  /// Lists broker accounts using the optional filters.
  [[nodiscard]] BrokerAccountsPage list_broker_accounts(const ListBrokerAccountsRequest& request = {});

  /// Retrieves a single broker account by identifier.
  [[nodiscard]] BrokerAccount get_broker_account(const std::string& account_id);

  /// Creates a new broker account.
  [[nodiscard]] BrokerAccount create_broker_account(const CreateBrokerAccountRequest& request);

  /// Updates an existing broker account with partial data.
  [[nodiscard]] BrokerAccount update_broker_account(const std::string& account_id, const UpdateBrokerAccountRequest& request);

  /// Closes an existing broker account.
  void delete_broker_account(const std::string& account_id);

  /// Lists documents attached to a broker account.
  [[nodiscard]] std::vector<AccountDocument> list_account_documents(const std::string& account_id);

  /// Uploads a new document for a broker account.
  [[nodiscard]] AccountDocument upload_account_document(const std::string& account_id, const CreateAccountDocumentRequest& request);

  /// Lists transfers initiated for the provided account.
  [[nodiscard]] TransfersPage list_account_transfers(const std::string& account_id, const ListTransfersRequest& request = {});

  /// Initiates a new transfer for the provided account.
  [[nodiscard]] Transfer create_account_transfer(const std::string& account_id, const CreateTransferRequest& request);

  /// Retrieves a transfer by identifier.
  [[nodiscard]] Transfer get_transfer(const std::string& transfer_id);

  /// Cancels a pending transfer by identifier.
  void cancel_transfer(const std::string& account_id, const std::string& transfer_id);

  /// Lists journals with the optional filters.
  [[nodiscard]] JournalsPage list_journals(const ListJournalsRequest& request = {});

  /// Creates a journal entry.
  [[nodiscard]] Journal create_journal(const CreateJournalRequest& request);

  /// Retrieves a journal entry by identifier.
  [[nodiscard]] Journal get_journal(const std::string& journal_id);

  /// Cancels a pending journal entry.
  void cancel_journal(const std::string& journal_id);

  /// Lists ACH relationships associated with the broker account.
  [[nodiscard]] BankRelationshipsPage list_ach_relationships(const std::string& account_id);

  /// Creates a new ACH relationship for the broker account.
  [[nodiscard]] BankRelationship create_ach_relationship(const std::string& account_id, const CreateAchRelationshipRequest& request);

  /// Deletes an existing ACH relationship.
  void delete_ach_relationship(const std::string& account_id, const std::string& relationship_id);

  /// Lists wire relationships for the broker account.
  [[nodiscard]] BankRelationshipsPage list_wire_relationships(const std::string& account_id);

  /// Creates a new wire relationship for the broker account.
  [[nodiscard]] BankRelationship create_wire_relationship(const std::string& account_id, const CreateWireRelationshipRequest& request);

  /// Deletes an existing wire relationship.
  void delete_wire_relationship(const std::string& account_id, const std::string& relationship_id);

 private:
  RestClient trading_client_;
  RestClient data_client_;
  RestClient data_beta_client_;
  RestClient broker_client_;
};

}  // namespace alpaca

