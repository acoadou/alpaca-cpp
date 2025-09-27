#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/BrokerClient.hpp"
#include "alpaca/Configuration.hpp"
#include "alpaca/MarketDataClient.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/TradingClient.hpp"
#include "alpaca/models/Account.hpp"
#include "alpaca/models/AccountActivity.hpp"
#include "alpaca/models/AccountConfiguration.hpp"
#include "alpaca/models/Asset.hpp"
#include "alpaca/models/Broker.hpp"
#include "alpaca/models/CalendarDay.hpp"
#include "alpaca/models/Clock.hpp"
#include "alpaca/models/CorporateActions.hpp"
#include "alpaca/models/MarketData.hpp"
#include "alpaca/models/News.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/PortfolioHistory.hpp"
#include "alpaca/models/Position.hpp"
#include "alpaca/models/Watchlist.hpp"

namespace alpaca {

/// High level client that exposes strongly typed wrappers around Alpaca REST
/// endpoints.
class AlpacaClient {
  public:
    explicit AlpacaClient(Configuration config, HttpClientPtr http_client = nullptr);
    AlpacaClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                 HttpClientPtr http_client = nullptr);

    /// Returns the trading domain client.
    [[nodiscard]] TradingClient& trading() noexcept;
    [[nodiscard]] TradingClient const& trading() const noexcept;

    /// Returns the market data domain client.
    [[nodiscard]] MarketDataClient& market_data() noexcept;
    [[nodiscard]] MarketDataClient const& market_data() const noexcept;

    /// Returns the broker domain client.
    [[nodiscard]] BrokerClient& broker() noexcept;
    [[nodiscard]] BrokerClient const& broker() const noexcept;

    /// Returns account information for the authenticated user.
    [[nodiscard]] Account get_account();

    /// Returns the account configuration for the authenticated user.
    [[nodiscard]] AccountConfiguration get_account_configuration();

    /// Updates account configuration fields using a partial request.
    [[nodiscard]] AccountConfiguration update_account_configuration(AccountConfigurationUpdateRequest const& request);

    /// Returns positions currently held in the authenticated account.
    [[nodiscard]] std::vector<Position> list_positions();

    /// Returns a single open position by symbol.
    [[nodiscard]] Position get_position(std::string const& symbol);

    /// Closes the position identified by \p symbol with optional close
    /// parameters.
    [[nodiscard]] Position close_position(std::string const& symbol, ClosePositionRequest const& request = {});

    /// Returns all orders that match the supplied filters.
    [[nodiscard]] std::vector<Order> list_orders(ListOrdersRequest const& request = {});

    /// Returns details for a single order.
    [[nodiscard]] Order get_order(std::string const& order_id);

    /// Returns an order using the provided client order identifier.
    [[nodiscard]] Order get_order_by_client_order_id(std::string const& client_order_id);

    /// Cancels the order identified by \p order_id.
    void cancel_order(std::string const& order_id);

    /// Cancels all open orders and returns the identifiers that were cancelled.
    [[nodiscard]] std::vector<CancelledOrderId> cancel_all_orders();

    /// Submits a new order and returns the created order representation.
    [[nodiscard]] Order submit_order(NewOrderRequest const& request);

    /// Replaces an existing order using the Alpaca replace endpoint.
    [[nodiscard]] Order replace_order(std::string const& order_id, ReplaceOrderRequest const& request);

    /// Returns the current trading clock.
    [[nodiscard]] Clock get_clock();

    /// Returns the trading calendar for the provided filters.
    [[nodiscard]] std::vector<CalendarDay> get_calendar(CalendarRequest const& request = {});

    /// Returns a list of tradable assets.
    [[nodiscard]] std::vector<Asset> list_assets(ListAssetsRequest const& request = {});

    /// Returns a single asset by symbol.
    [[nodiscard]] Asset get_asset(std::string const& symbol);

    /// Returns account activities matching the optional filters.
    [[nodiscard]] std::vector<AccountActivity> get_account_activities(AccountActivitiesRequest const& request = {});

    /// Returns the account's portfolio history for the optional request.
    [[nodiscard]] PortfolioHistory get_portfolio_history(PortfolioHistoryRequest const& request = {});

    /// Returns all watchlists available for the account.
    [[nodiscard]] std::vector<Watchlist> list_watchlists();

    /// Returns a single watchlist by identifier.
    [[nodiscard]] Watchlist get_watchlist(std::string const& id);

    /// Returns a single watchlist by name.
    [[nodiscard]] Watchlist get_watchlist_by_name(std::string const& name);

    /// Creates a new watchlist.
    [[nodiscard]] Watchlist create_watchlist(CreateWatchlistRequest const& request);

    /// Updates an existing watchlist.
    [[nodiscard]] Watchlist update_watchlist(std::string const& id, UpdateWatchlistRequest const& request);

    /// Adds an asset to the watchlist.
    [[nodiscard]] Watchlist add_asset_to_watchlist(std::string const& id, std::string const& symbol);

    /// Removes an asset from the watchlist.
    [[nodiscard]] Watchlist remove_asset_from_watchlist(std::string const& id, std::string const& symbol);

    /// Deletes an existing watchlist.
    void delete_watchlist(std::string const& id);

    /// Returns the latest reported trade for the supplied stock symbol.
    [[nodiscard]] LatestStockTrade get_latest_stock_trade(std::string const& symbol);

    /// Returns the latest NBBO quote for the supplied stock symbol.
    [[nodiscard]] LatestStockQuote get_latest_stock_quote(std::string const& symbol);

    /// Returns historical bars for the supplied stock symbol using optional
    /// filters.
    [[nodiscard]] StockBars get_stock_bars(std::string const& symbol, StockBarsRequest const& request = {});

    /// Returns all available historical bars for the supplied stock symbol,
    /// automatically traversing pagination.
    [[nodiscard]] std::vector<StockBar> get_all_stock_bars(std::string const& symbol, StockBarsRequest request = {});

    /// Returns a single-pass range that yields stock bars across every page the API exposes.
    [[nodiscard]] PaginatedVectorRange<StockBarsRequest, StockBars, StockBar>
    stock_bars_range(std::string const& symbol, StockBarsRequest request = {}) const;

    /// Returns a consolidated market data snapshot for the supplied stock
    /// symbol.
    [[nodiscard]] StockSnapshot get_stock_snapshot(std::string const& symbol);

    /// Retrieves Alpaca news articles using the optional filters supplied in \p
    /// params.
    [[nodiscard]] NewsResponse get_news(NewsRequest const& request = {});

    /// Returns a range that streams news articles respecting server rate limits.
    [[nodiscard]] PaginatedVectorRange<NewsRequest, NewsResponse, NewsArticle>
    news_range(NewsRequest request = {}) const;

    /// Retrieves corporate action announcements with optional filters.
    [[nodiscard]] CorporateActionAnnouncementsResponse
    get_corporate_announcements(CorporateActionAnnouncementsRequest const& request = {});

    /// Retrieves corporate action events with optional filters.
    [[nodiscard]] CorporateActionEventsResponse get_corporate_actions(CorporateActionEventsRequest const& request = {});

    /// Retrieves multi-symbol stock aggregates from the Alpaca market data API.
    [[nodiscard]] MultiStockBars get_stock_aggregates(MultiStockBarsRequest const& request);

    /// Retrieves multi-symbol stock quotes from the Alpaca market data API.
    [[nodiscard]] MultiStockQuotes get_stock_quotes(MultiStockQuotesRequest const& request);

    /// Retrieves multi-symbol stock trades from the Alpaca market data API.
    [[nodiscard]] MultiStockTrades get_stock_trades(MultiStockTradesRequest const& request);

    /// Retrieves options aggregates across one or more contracts.
    [[nodiscard]] MultiOptionBars get_option_aggregates(MultiOptionBarsRequest const& request);

    /// Retrieves the latest options quotes across one or more contracts.
    [[nodiscard]] MultiOptionQuotes get_option_quotes(MultiOptionQuotesRequest const& request);

    /// Retrieves the latest options trades across one or more contracts.
    [[nodiscard]] MultiOptionTrades get_option_trades(MultiOptionTradesRequest const& request);

    /// Retrieves crypto aggregates across one or more symbols.
    [[nodiscard]] MultiCryptoBars get_crypto_aggregates(MultiCryptoBarsRequest const& request);

    /// Retrieves crypto quotes across one or more symbols.
    [[nodiscard]] MultiCryptoQuotes get_crypto_quotes(MultiCryptoQuotesRequest const& request);

    /// Retrieves crypto trades across one or more symbols.
    [[nodiscard]] MultiCryptoTrades get_crypto_trades(MultiCryptoTradesRequest const& request);

    /// Lists broker accounts using the optional filters.
    [[nodiscard]] BrokerAccountsPage list_broker_accounts(ListBrokerAccountsRequest const& request = {});
    [[nodiscard]] PaginatedVectorRange<ListBrokerAccountsRequest, BrokerAccountsPage, BrokerAccount>
    list_broker_accounts_range(ListBrokerAccountsRequest request = {}) const;

    /// Retrieves a single broker account by identifier.
    [[nodiscard]] BrokerAccount get_broker_account(std::string const& account_id);

    /// Creates a new broker account.
    [[nodiscard]] BrokerAccount create_broker_account(CreateBrokerAccountRequest const& request);

    /// Updates an existing broker account with partial data.
    [[nodiscard]] BrokerAccount update_broker_account(std::string const& account_id,
                                                      UpdateBrokerAccountRequest const& request);

    /// Closes an existing broker account.
    void delete_broker_account(std::string const& account_id);

    /// Lists documents attached to a broker account.
    [[nodiscard]] std::vector<AccountDocument> list_account_documents(std::string const& account_id);

    /// Uploads a new document for a broker account.
    [[nodiscard]] AccountDocument upload_account_document(std::string const& account_id,
                                                          CreateAccountDocumentRequest const& request);

    /// Lists transfers initiated for the provided account.
    [[nodiscard]] TransfersPage list_account_transfers(std::string const& account_id,
                                                       ListTransfersRequest const& request = {});
    [[nodiscard]] PaginatedVectorRange<ListTransfersRequest, TransfersPage, Transfer>
    list_account_transfers_range(std::string const& account_id, ListTransfersRequest request = {}) const;

    /// Initiates a new transfer for the provided account.
    [[nodiscard]] Transfer create_account_transfer(std::string const& account_id, CreateTransferRequest const& request);

    /// Retrieves a transfer by identifier.
    [[nodiscard]] Transfer get_transfer(std::string const& transfer_id);

    /// Cancels a pending transfer by identifier.
    void cancel_transfer(std::string const& account_id, std::string const& transfer_id);

    /// Lists journals with the optional filters.
    [[nodiscard]] JournalsPage list_journals(ListJournalsRequest const& request = {});
    [[nodiscard]] PaginatedVectorRange<ListJournalsRequest, JournalsPage, Journal>
    list_journals_range(ListJournalsRequest request = {}) const;

    /// Creates a journal entry.
    [[nodiscard]] Journal create_journal(CreateJournalRequest const& request);

    /// Retrieves a journal entry by identifier.
    [[nodiscard]] Journal get_journal(std::string const& journal_id);

    /// Cancels a pending journal entry.
    void cancel_journal(std::string const& journal_id);

    /// Lists ACH relationships associated with the broker account.
    [[nodiscard]] BankRelationshipsPage list_ach_relationships(std::string const& account_id);

    /// Creates a new ACH relationship for the broker account.
    [[nodiscard]] BankRelationship create_ach_relationship(std::string const& account_id,
                                                           CreateAchRelationshipRequest const& request);

    /// Deletes an existing ACH relationship.
    void delete_ach_relationship(std::string const& account_id, std::string const& relationship_id);

    /// Lists wire relationships for the broker account.
    [[nodiscard]] BankRelationshipsPage list_wire_relationships(std::string const& account_id);

    /// Creates a new wire relationship for the broker account.
    [[nodiscard]] BankRelationship create_wire_relationship(std::string const& account_id,
                                                            CreateWireRelationshipRequest const& request);

    /// Deletes an existing wire relationship.
    void delete_wire_relationship(std::string const& account_id, std::string const& relationship_id);

  private:
    Configuration config_;
    HttpClientPtr http_client_;
    TradingClient trading_client_;
    MarketDataClient market_data_client_;
    BrokerClient broker_client_;
};

} // namespace alpaca
