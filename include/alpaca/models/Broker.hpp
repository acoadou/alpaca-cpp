#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"

namespace alpaca {

struct Address {
    std::vector<std::string> street_address;
    std::string city;
    std::string state;
    std::string postal_code;
    std::string country;
};

struct ContactInformation {
    std::string email_address;
    std::string phone_number;
    Address address;
};

struct IdentityInformation {
    std::string given_name;
    std::string family_name;
    std::optional<std::string> middle_name;
    std::string date_of_birth;
    std::string country_of_tax_residence;
    std::string tax_id_type;
    std::string tax_id;
    std::string country_of_citizenship;
};

struct EmploymentInformation {
    std::string status;
    std::optional<std::string> employer_name;
    std::optional<std::string> employer_address;
    std::optional<std::string> position;
    std::optional<std::string> source_of_funds;
    std::optional<std::string> annual_income_min;
    std::optional<std::string> annual_income_max;
    std::optional<std::string> liquid_net_worth_min;
    std::optional<std::string> liquid_net_worth_max;
    std::optional<std::string> total_net_worth_min;
    std::optional<std::string> total_net_worth_max;
};

struct DisclosureInformation {
    bool is_control_person{false};
    bool is_affiliated_exchange_or_finra{false};
    bool is_politically_exposed{false};
    bool immediate_family_exposed{false};
};

struct TrustedContactInformation {
    std::string given_name;
    std::string family_name;
    std::optional<std::string> email_address;
    std::optional<std::string> phone_number;
    std::optional<Address> address;
};

struct AccountAgreement {
    std::string agreement;
    std::string signed_at;
    std::string ip_address;
};

struct AccountDocument {
    std::string id;
    std::string document_type;
    std::optional<std::string> document_sub_type;
    std::string status;
    std::string created_at;
    std::optional<std::string> updated_at;
    std::optional<std::string> uploaded_at;
    std::optional<std::string> rejected_at;
    std::optional<std::string> rejection_reason;
};

struct CreateAccountDocumentRequest {
    std::string document_type;
    std::optional<std::string> document_sub_type;
    std::string content;
    std::string mime_type;
    std::optional<std::string> name;
};

struct BrokerAccount {
    std::string id;
    std::string account_number;
    std::string status;
    std::string created_at;
    std::optional<std::string> updated_at;
    std::optional<std::string> last_equity;
    std::optional<std::string> cash;
    ContactInformation contact;
    IdentityInformation identity;
    EmploymentInformation employment;
    DisclosureInformation disclosures;
    std::optional<TrustedContactInformation> trusted_contact;
    std::vector<AccountAgreement> agreements;
    std::vector<AccountDocument> documents;
};

struct CreateBrokerAccountRequest {
    ContactInformation contact;
    IdentityInformation identity;
    EmploymentInformation employment;
    DisclosureInformation disclosures;
    std::vector<AccountAgreement> agreements;
    std::vector<CreateAccountDocumentRequest> documents;
    std::optional<TrustedContactInformation> trusted_contact;
};

struct UpdateBrokerAccountRequest {
    std::optional<ContactInformation> contact;
    std::optional<IdentityInformation> identity;
    std::optional<EmploymentInformation> employment;
    std::optional<DisclosureInformation> disclosures;
    std::optional<TrustedContactInformation> trusted_contact;
};

struct ListBrokerAccountsRequest {
    std::optional<std::string> status;
    std::optional<std::string> entity_type;
    std::optional<int> page_size;
    std::optional<std::string> next_page_token;

    [[nodiscard]] QueryParams to_query_params() const;
};

struct BrokerAccountsPage {
    std::vector<BrokerAccount> accounts;
    std::optional<std::string> next_page_token;
};

struct Transfer {
    std::string id;
    std::string account_id;
    std::string status;
    std::string type;
    std::string direction;
    std::string amount;
    std::optional<std::string> relationship_id;
    std::optional<std::string> reason;
    std::string created_at;
    std::optional<std::string> updated_at;
    std::optional<std::string> completed_at;
    std::optional<std::string> expires_at;
};

struct CreateTransferRequest {
    std::string type;
    std::string direction;
    std::string amount;
    std::optional<std::string> timing;
    std::optional<std::string> relationship_id;
    std::optional<std::string> transfer_id;
};

struct ListTransfersRequest {
    std::optional<std::string> status;
    std::optional<std::string> direction;
    std::optional<std::string> type;
    std::optional<int> page_size;
    std::optional<std::string> next_page_token;

    [[nodiscard]] QueryParams to_query_params() const;
};

struct TransfersPage {
    std::vector<Transfer> transfers;
    std::optional<std::string> next_page_token;
};

struct Journal {
    std::string id;
    std::string status;
    std::string entry_type;
    std::string description;
    std::string amount;
    std::string created_at;
    std::optional<std::string> updated_at;
    std::optional<std::string> debit_account_id;
    std::optional<std::string> credit_account_id;
    std::optional<std::string> reason;
};

struct CreateJournalRequest {
    std::string entry_type;
    std::string description;
    std::string amount;
    std::optional<std::string> debit_account_id;
    std::optional<std::string> credit_account_id;
};

struct ListJournalsRequest {
    std::optional<std::string> status;
    std::optional<std::string> entry_type;
    std::optional<int> page_size;
    std::optional<std::string> next_page_token;

    [[nodiscard]] QueryParams to_query_params() const;
};

struct JournalsPage {
    std::vector<Journal> journals;
    std::optional<std::string> next_page_token;
};

struct BankRelationship {
    std::string id;
    std::string account_id;
    std::string status;
    std::string bank_account_type;
    std::optional<std::string> nickname;
    std::optional<std::string> bank_name;
    std::optional<std::string> account_owner_name;
    std::optional<std::string> created_at;
};

struct CreateAchRelationshipRequest {
    std::string bank_account_type;
    std::string routing_number;
    std::string account_number;
    std::optional<std::string> nickname;
    std::optional<std::string> account_owner_name;
};

struct CreateWireRelationshipRequest {
    std::string bank_account_type;
    std::string beneficiary_bank;
    std::string beneficiary_account;
    std::optional<std::string> nickname;
    std::optional<std::string> account_owner_name;
};

struct BankRelationshipsPage {
    std::vector<BankRelationship> relationships;
    std::optional<std::string> next_page_token;
};

void to_json(Json& j, Address const& value);
void from_json(Json const& j, Address& value);

void to_json(Json& j, ContactInformation const& value);
void from_json(Json const& j, ContactInformation& value);

void to_json(Json& j, IdentityInformation const& value);
void from_json(Json const& j, IdentityInformation& value);

void to_json(Json& j, EmploymentInformation const& value);
void from_json(Json const& j, EmploymentInformation& value);

void to_json(Json& j, DisclosureInformation const& value);
void from_json(Json const& j, DisclosureInformation& value);

void to_json(Json& j, TrustedContactInformation const& value);
void from_json(Json const& j, TrustedContactInformation& value);

void to_json(Json& j, AccountAgreement const& value);
void from_json(Json const& j, AccountAgreement& value);

void to_json(Json& j, AccountDocument const& value);
void from_json(Json const& j, AccountDocument& value);

void to_json(Json& j, CreateAccountDocumentRequest const& value);

void to_json(Json& j, CreateBrokerAccountRequest const& value);
void to_json(Json& j, UpdateBrokerAccountRequest const& value);
void from_json(Json const& j, BrokerAccount& value);

void from_json(Json const& j, BrokerAccountsPage& value);

void to_json(Json& j, CreateTransferRequest const& value);
void from_json(Json const& j, Transfer& value);
void from_json(Json const& j, TransfersPage& value);

void to_json(Json& j, CreateJournalRequest const& value);
void from_json(Json const& j, Journal& value);
void from_json(Json const& j, JournalsPage& value);

void to_json(Json& j, CreateAchRelationshipRequest const& value);
void to_json(Json& j, CreateWireRelationshipRequest const& value);
void from_json(Json const& j, BankRelationship& value);
void from_json(Json const& j, BankRelationshipsPage& value);

} // namespace alpaca
