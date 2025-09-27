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
    BrokerClient(Configuration const& config, HttpClientPtr http_client = nullptr);
    BrokerClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                 HttpClientPtr http_client = nullptr);

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

  private:
    RestClient rest_client_;
};

} // namespace alpaca
