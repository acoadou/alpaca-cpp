#pragma once

#include <string>
#include <vector>

#include "alpaca/Configuration.hpp"
#include "alpaca/Pagination.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Broker.hpp"

namespace alpaca {

/// Broker client exposes account onboarding, documents, journals, and transfers.
class BrokerClient {
  public:
    BrokerClient(Configuration const& config, HttpClientPtr http_client = nullptr,
                 RestClient::Options options = RestClient::default_options());
    BrokerClient(Configuration const& config, RestClient::Options options);
    BrokerClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                 HttpClientPtr http_client = nullptr, RestClient::Options options = RestClient::default_options());
    BrokerClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                 RestClient::Options options);

    [[nodiscard]] BrokerAccountsPage list_accounts(ListBrokerAccountsRequest const& request = {}) const;
    [[nodiscard]] BrokerAccount get_account(std::string const& account_id) const;
    [[nodiscard]] BrokerAccount create_account(CreateBrokerAccountRequest const& request) const;
    [[nodiscard]] BrokerAccount update_account(std::string const& account_id,
                                               UpdateBrokerAccountRequest const& request) const;
    void close_account(std::string const& account_id) const;

    [[nodiscard]] PaginatedVectorRange<ListBrokerAccountsRequest, BrokerAccountsPage, BrokerAccount>
    list_accounts_range(ListBrokerAccountsRequest request = {}) const;

    [[nodiscard]] std::vector<AccountDocument> list_documents(std::string const& account_id) const;
    [[nodiscard]] AccountDocument upload_document(std::string const& account_id,
                                                  CreateAccountDocumentRequest const& request) const;

    [[nodiscard]] TransfersPage list_transfers(std::string const& account_id,
                                               ListTransfersRequest const& request = {}) const;
    [[nodiscard]] Transfer create_transfer(std::string const& account_id, CreateTransferRequest const& request) const;
    [[nodiscard]] Transfer get_transfer(std::string const& transfer_id) const;
    void cancel_transfer(std::string const& account_id, std::string const& transfer_id) const;

    [[nodiscard]] PaginatedVectorRange<ListTransfersRequest, TransfersPage, Transfer>
    list_transfers_range(std::string const& account_id, ListTransfersRequest request = {}) const;

    [[nodiscard]] JournalsPage list_journals(ListJournalsRequest const& request = {}) const;
    [[nodiscard]] Journal create_journal(CreateJournalRequest const& request) const;
    [[nodiscard]] Journal get_journal(std::string const& journal_id) const;
    void cancel_journal(std::string const& journal_id) const;

    [[nodiscard]] PaginatedVectorRange<ListJournalsRequest, JournalsPage, Journal>
    list_journals_range(ListJournalsRequest request = {}) const;

    [[nodiscard]] BankRelationshipsPage list_ach_relationships(std::string const& account_id) const;
    [[nodiscard]] BankRelationship create_ach_relationship(std::string const& account_id,
                                                           CreateAchRelationshipRequest const& request) const;
    void delete_ach_relationship(std::string const& account_id, std::string const& relationship_id) const;

    [[nodiscard]] BankRelationshipsPage list_wire_relationships(std::string const& account_id) const;
    [[nodiscard]] BankRelationship create_wire_relationship(std::string const& account_id,
                                                            CreateWireRelationshipRequest const& request) const;
    void delete_wire_relationship(std::string const& account_id, std::string const& relationship_id) const;

    [[nodiscard]] std::vector<BrokerWatchlist> list_watchlists(std::string const& account_id) const;
    [[nodiscard]] BrokerWatchlist get_watchlist(std::string const& account_id, std::string const& watchlist_id) const;
    [[nodiscard]] BrokerWatchlist create_watchlist(std::string const& account_id,
                                                   CreateBrokerWatchlistRequest const& request) const;
    [[nodiscard]] BrokerWatchlist update_watchlist(std::string const& account_id, std::string const& watchlist_id,
                                                   UpdateBrokerWatchlistRequest const& request) const;
    [[nodiscard]] BrokerWatchlist add_asset_to_watchlist(std::string const& account_id, std::string const& watchlist_id,
                                                         std::string const& symbol) const;
    [[nodiscard]] BrokerWatchlist remove_asset_from_watchlist(std::string const& account_id,
                                                              std::string const& watchlist_id,
                                                              std::string const& symbol) const;
    void delete_watchlist(std::string const& account_id, std::string const& watchlist_id) const;

    [[nodiscard]] std::vector<RebalancingPortfolio>
    list_rebalancing_portfolios(ListRebalancingPortfoliosRequest const& request = {}) const;
    [[nodiscard]] RebalancingPortfolio get_rebalancing_portfolio(std::string const& portfolio_id) const;
    [[nodiscard]] RebalancingPortfolio
    create_rebalancing_portfolio(CreateRebalancingPortfolioRequest const& request) const;
    [[nodiscard]] RebalancingPortfolio
    update_rebalancing_portfolio(std::string const& portfolio_id,
                                 UpdateRebalancingPortfolioRequest const& request) const;
    void deactivate_rebalancing_portfolio(std::string const& portfolio_id) const;

    [[nodiscard]] RebalancingSubscriptionsPage
    list_rebalancing_subscriptions(ListRebalancingSubscriptionsRequest const& request = {}) const;
    [[nodiscard]] PaginatedVectorRange<ListRebalancingSubscriptionsRequest, RebalancingSubscriptionsPage,
                                       RebalancingSubscription>
    list_rebalancing_subscriptions_range(ListRebalancingSubscriptionsRequest request = {}) const;
    [[nodiscard]] RebalancingSubscription get_rebalancing_subscription(std::string const& subscription_id) const;
    [[nodiscard]] RebalancingSubscription
    create_rebalancing_subscription(CreateRebalancingSubscriptionRequest const& request) const;

    [[nodiscard]] ManagedPortfolioHistory
    get_managed_portfolio_history(std::string const& account_id,
                                  ManagedPortfolioHistoryRequest const& request = {}) const;

    [[nodiscard]] BrokerEventsPage list_events(ListBrokerEventsRequest const& request = {}) const;
    [[nodiscard]] PaginatedVectorRange<ListBrokerEventsRequest, BrokerEventsPage, BrokerEvent>
    list_events_range(ListBrokerEventsRequest request = {}) const;

    [[nodiscard]] BrokerWebhookSubscriptionsPage
    list_webhook_subscriptions(ListBrokerWebhookSubscriptionsRequest const& request = {}) const;
    [[nodiscard]] BrokerWebhookSubscription
    create_webhook_subscription(CreateBrokerWebhookSubscriptionRequest const& request) const;
    [[nodiscard]] BrokerWebhookSubscription
    update_webhook_subscription(std::string const& subscription_id,
                                UpdateBrokerWebhookSubscriptionRequest const& request) const;
    void delete_webhook_subscription(std::string const& subscription_id) const;

  private:
    RestClient rest_client_;
};

} // namespace alpaca
