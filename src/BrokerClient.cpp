#include "alpaca/BrokerClient.hpp"

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

Json to_json_payload(CreateBrokerWatchlistRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(UpdateBrokerWatchlistRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateRebalancingPortfolioRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(UpdateRebalancingPortfolioRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateRebalancingSubscriptionRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(CreateBrokerWebhookSubscriptionRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

Json to_json_payload(UpdateBrokerWebhookSubscriptionRequest const& request) {
    Json payload;
    to_json(payload, request);
    return payload;
}

} // namespace

BrokerClient::BrokerClient(Configuration const& config, HttpClientPtr http_client)
  : rest_client_(config, ensure_http_client(http_client), config.broker_base_url) {
}

BrokerClient::BrokerClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                           HttpClientPtr http_client)
  : BrokerClient(Configuration::FromEnvironment(environment, std::move(api_key_id), std::move(api_secret_key)),
                 std::move(http_client)) {
}

BrokerAccountsPage BrokerClient::list_accounts(ListBrokerAccountsRequest const& request) const {
    return rest_client_.get<BrokerAccountsPage>("/v1/accounts", request.to_query_params());
}

BrokerAccount BrokerClient::get_account(std::string const& account_id) const {
    return rest_client_.get<BrokerAccount>("/v1/accounts/" + account_id);
}

BrokerAccount BrokerClient::create_account(CreateBrokerAccountRequest const& request) const {
    return rest_client_.post<BrokerAccount>("/v1/accounts", to_json_payload(request));
}

BrokerAccount BrokerClient::update_account(std::string const& account_id,
                                           UpdateBrokerAccountRequest const& request) const {
    return rest_client_.patch<BrokerAccount>("/v1/accounts/" + account_id, to_json_payload(request));
}

void BrokerClient::close_account(std::string const& account_id) const {
    rest_client_.post<void>("/v1/accounts/" + account_id + "/actions/close", Json::object());
}

PaginatedVectorRange<ListBrokerAccountsRequest, BrokerAccountsPage, BrokerAccount>
BrokerClient::list_accounts_range(ListBrokerAccountsRequest request) const {
    return PaginatedVectorRange<ListBrokerAccountsRequest, BrokerAccountsPage, BrokerAccount>(
    std::move(request),
    [this](ListBrokerAccountsRequest const& req) {
        return list_accounts(req);
    },
    [](BrokerAccountsPage const& page) -> std::vector<BrokerAccount> const& {
        return page.accounts;
    },
    [](BrokerAccountsPage const& page) {
        return page.next_page_token;
    },
    [](ListBrokerAccountsRequest& req, std::optional<std::string> const& token) {
        req.next_page_token = token;
    });
}

std::vector<AccountDocument> BrokerClient::list_documents(std::string const& account_id) const {
    return rest_client_.get<std::vector<AccountDocument>>("/v1/accounts/" + account_id + "/documents");
}

AccountDocument BrokerClient::upload_document(std::string const& account_id,
                                              CreateAccountDocumentRequest const& request) const {
    return rest_client_.post<AccountDocument>("/v1/accounts/" + account_id + "/documents", to_json_payload(request));
}

TransfersPage BrokerClient::list_transfers(std::string const& account_id, ListTransfersRequest const& request) const {
    return rest_client_.get<TransfersPage>("/v1/accounts/" + account_id + "/transfers", request.to_query_params());
}

Transfer BrokerClient::create_transfer(std::string const& account_id, CreateTransferRequest const& request) const {
    return rest_client_.post<Transfer>("/v1/accounts/" + account_id + "/transfers", to_json_payload(request));
}

Transfer BrokerClient::get_transfer(std::string const& transfer_id) const {
    return rest_client_.get<Transfer>("/v1/transfers/" + transfer_id);
}

void BrokerClient::cancel_transfer(std::string const& account_id, std::string const& transfer_id) const {
    rest_client_.del<void>("/v1/accounts/" + account_id + "/transfers/" + transfer_id);
}

PaginatedVectorRange<ListTransfersRequest, TransfersPage, Transfer>
BrokerClient::list_transfers_range(std::string const& account_id, ListTransfersRequest request) const {
    return PaginatedVectorRange<ListTransfersRequest, TransfersPage, Transfer>(
    std::move(request),
    [this, account_id](ListTransfersRequest const& req) {
        return list_transfers(account_id, req);
    },
    [](TransfersPage const& page) -> std::vector<Transfer> const& {
        return page.transfers;
    },
    [](TransfersPage const& page) {
        return page.next_page_token;
    },
    [](ListTransfersRequest& req, std::optional<std::string> const& token) {
        req.next_page_token = token;
    });
}

JournalsPage BrokerClient::list_journals(ListJournalsRequest const& request) const {
    return rest_client_.get<JournalsPage>("/v1/journals", request.to_query_params());
}

Journal BrokerClient::create_journal(CreateJournalRequest const& request) const {
    return rest_client_.post<Journal>("/v1/journals", to_json_payload(request));
}

Journal BrokerClient::get_journal(std::string const& journal_id) const {
    return rest_client_.get<Journal>("/v1/journals/" + journal_id);
}

void BrokerClient::cancel_journal(std::string const& journal_id) const {
    rest_client_.del<void>("/v1/journals/" + journal_id);
}

PaginatedVectorRange<ListJournalsRequest, JournalsPage, Journal>
BrokerClient::list_journals_range(ListJournalsRequest request) const {
    return PaginatedVectorRange<ListJournalsRequest, JournalsPage, Journal>(
    std::move(request),
    [this](ListJournalsRequest const& req) {
        return list_journals(req);
    },
    [](JournalsPage const& page) -> std::vector<Journal> const& {
        return page.journals;
    },
    [](JournalsPage const& page) {
        return page.next_page_token;
    },
    [](ListJournalsRequest& req, std::optional<std::string> const& token) {
        req.next_page_token = token;
    });
}

BankRelationshipsPage BrokerClient::list_ach_relationships(std::string const& account_id) const {
    return rest_client_.get<BankRelationshipsPage>("/v1/accounts/" + account_id + "/ach_relationships");
}

BankRelationship BrokerClient::create_ach_relationship(std::string const& account_id,
                                                       CreateAchRelationshipRequest const& request) const {
    return rest_client_.post<BankRelationship>("/v1/accounts/" + account_id + "/ach_relationships",
                                               to_json_payload(request));
}

void BrokerClient::delete_ach_relationship(std::string const& account_id, std::string const& relationship_id) const {
    rest_client_.del<void>("/v1/accounts/" + account_id + "/ach_relationships/" + relationship_id);
}

BankRelationshipsPage BrokerClient::list_wire_relationships(std::string const& account_id) const {
    return rest_client_.get<BankRelationshipsPage>("/v1/accounts/" + account_id + "/wire_relationships");
}

BankRelationship BrokerClient::create_wire_relationship(std::string const& account_id,
                                                        CreateWireRelationshipRequest const& request) const {
    return rest_client_.post<BankRelationship>("/v1/accounts/" + account_id + "/wire_relationships",
                                               to_json_payload(request));
}

void BrokerClient::delete_wire_relationship(std::string const& account_id, std::string const& relationship_id) const {
    rest_client_.del<void>("/v1/accounts/" + account_id + "/wire_relationships/" + relationship_id);
}

std::vector<BrokerWatchlist> BrokerClient::list_watchlists(std::string const& account_id) const {
    return rest_client_.get<std::vector<BrokerWatchlist>>("/trading/accounts/" + account_id + "/watchlists");
}

BrokerWatchlist BrokerClient::get_watchlist(std::string const& account_id, std::string const& watchlist_id) const {
    return rest_client_.get<BrokerWatchlist>("/trading/accounts/" + account_id + "/watchlists/" + watchlist_id);
}

BrokerWatchlist BrokerClient::create_watchlist(std::string const& account_id,
                                               CreateBrokerWatchlistRequest const& request) const {
    return rest_client_.post<BrokerWatchlist>("/trading/accounts/" + account_id + "/watchlists",
                                              to_json_payload(request));
}

BrokerWatchlist BrokerClient::update_watchlist(std::string const& account_id, std::string const& watchlist_id,
                                               UpdateBrokerWatchlistRequest const& request) const {
    return rest_client_.put<BrokerWatchlist>("/trading/accounts/" + account_id + "/watchlists/" + watchlist_id,
                                             to_json_payload(request));
}

BrokerWatchlist BrokerClient::add_asset_to_watchlist(std::string const& account_id, std::string const& watchlist_id,
                                                     std::string const& symbol) const {
    Json payload = Json::object();
    payload["symbol"] = symbol;
    return rest_client_.post<BrokerWatchlist>("/trading/accounts/" + account_id + "/watchlists/" + watchlist_id,
                                              payload);
}

BrokerWatchlist BrokerClient::remove_asset_from_watchlist(std::string const& account_id,
                                                          std::string const& watchlist_id,
                                                          std::string const& symbol) const {
    return rest_client_.del<BrokerWatchlist>("/trading/accounts/" + account_id + "/watchlists/" + watchlist_id + "/" +
                                             symbol);
}

void BrokerClient::delete_watchlist(std::string const& account_id, std::string const& watchlist_id) const {
    rest_client_.del<void>("/trading/accounts/" + account_id + "/watchlists/" + watchlist_id);
}

std::vector<RebalancingPortfolio>
BrokerClient::list_rebalancing_portfolios(ListRebalancingPortfoliosRequest const& request) const {
    Json response = rest_client_.get<Json>("/rebalancing/portfolios", request.to_query_params());
    if (response.is_array()) {
        return response.get<std::vector<RebalancingPortfolio>>();
    }
    if (response.contains("portfolios")) {
        return response.at("portfolios").get<std::vector<RebalancingPortfolio>>();
    }
    return {};
}

RebalancingPortfolio BrokerClient::get_rebalancing_portfolio(std::string const& portfolio_id) const {
    return rest_client_.get<RebalancingPortfolio>("/rebalancing/portfolios/" + portfolio_id);
}

RebalancingPortfolio
BrokerClient::create_rebalancing_portfolio(CreateRebalancingPortfolioRequest const& request) const {
    return rest_client_.post<RebalancingPortfolio>("/rebalancing/portfolios", to_json_payload(request));
}

RebalancingPortfolio
BrokerClient::update_rebalancing_portfolio(std::string const& portfolio_id,
                                           UpdateRebalancingPortfolioRequest const& request) const {
    return rest_client_.patch<RebalancingPortfolio>("/rebalancing/portfolios/" + portfolio_id,
                                                    to_json_payload(request));
}

void BrokerClient::deactivate_rebalancing_portfolio(std::string const& portfolio_id) const {
    rest_client_.del<void>("/rebalancing/portfolios/" + portfolio_id);
}

RebalancingSubscriptionsPage
BrokerClient::list_rebalancing_subscriptions(ListRebalancingSubscriptionsRequest const& request) const {
    return rest_client_.get<RebalancingSubscriptionsPage>("/rebalancing/subscriptions", request.to_query_params());
}

PaginatedVectorRange<ListRebalancingSubscriptionsRequest, RebalancingSubscriptionsPage, RebalancingSubscription>
BrokerClient::list_rebalancing_subscriptions_range(ListRebalancingSubscriptionsRequest request) const {
    return PaginatedVectorRange<ListRebalancingSubscriptionsRequest, RebalancingSubscriptionsPage,
                                RebalancingSubscription>(
    std::move(request),
    [this](ListRebalancingSubscriptionsRequest const& req) {
        return list_rebalancing_subscriptions(req);
    },
    [](RebalancingSubscriptionsPage const& page) -> std::vector<RebalancingSubscription> const& {
        return page.subscriptions;
    },
    [](RebalancingSubscriptionsPage const& page) {
        return page.next_page_token;
    },
    [](ListRebalancingSubscriptionsRequest& req, std::optional<std::string> const& token) {
        req.page_token = token;
    });
}

RebalancingSubscription BrokerClient::get_rebalancing_subscription(std::string const& subscription_id) const {
    return rest_client_.get<RebalancingSubscription>("/rebalancing/subscriptions/" + subscription_id);
}

RebalancingSubscription
BrokerClient::create_rebalancing_subscription(CreateRebalancingSubscriptionRequest const& request) const {
    return rest_client_.post<RebalancingSubscription>("/rebalancing/subscriptions", to_json_payload(request));
}

ManagedPortfolioHistory
BrokerClient::get_managed_portfolio_history(std::string const& account_id,
                                            ManagedPortfolioHistoryRequest const& request) const {
    return rest_client_.get<ManagedPortfolioHistory>("/trading/accounts/" + account_id + "/account/portfolio/history",
                                                     request.to_query_params());
}

BrokerEventsPage BrokerClient::list_events(ListBrokerEventsRequest const& request) const {
    return rest_client_.get<BrokerEventsPage>("/v1/events", request.to_query_params());
}

PaginatedVectorRange<ListBrokerEventsRequest, BrokerEventsPage, BrokerEvent>
BrokerClient::list_events_range(ListBrokerEventsRequest request) const {
    return PaginatedVectorRange<ListBrokerEventsRequest, BrokerEventsPage, BrokerEvent>(
    std::move(request),
    [this](ListBrokerEventsRequest const& req) {
        return list_events(req);
    },
    [](BrokerEventsPage const& page) -> std::vector<BrokerEvent> const& {
        return page.events;
    },
    [](BrokerEventsPage const& page) {
        return page.next_page_token;
    },
    [](ListBrokerEventsRequest& req, std::optional<std::string> const& token) {
        req.page_token = token;
    });
}

BrokerWebhookSubscriptionsPage
BrokerClient::list_webhook_subscriptions(ListBrokerWebhookSubscriptionsRequest const& request) const {
    return rest_client_.get<BrokerWebhookSubscriptionsPage>("/v1/events/subscriptions", request.to_query_params());
}

BrokerWebhookSubscription
BrokerClient::create_webhook_subscription(CreateBrokerWebhookSubscriptionRequest const& request) const {
    return rest_client_.post<BrokerWebhookSubscription>("/v1/events/subscriptions", to_json_payload(request));
}

BrokerWebhookSubscription
BrokerClient::update_webhook_subscription(std::string const& subscription_id,
                                          UpdateBrokerWebhookSubscriptionRequest const& request) const {
    return rest_client_.patch<BrokerWebhookSubscription>("/v1/events/subscriptions/" + subscription_id,
                                                         to_json_payload(request));
}

void BrokerClient::delete_webhook_subscription(std::string const& subscription_id) const {
    rest_client_.del<void>("/v1/events/subscriptions/" + subscription_id);
}

} // namespace alpaca
