#include "alpaca/models/Broker.hpp"

#include <utility>

namespace alpaca {
namespace {

template <typename T> void assign_optional(Json const& json, char const *key, std::optional<T>& target) {
    if (json.contains(key) && !json.at(key).is_null()) {
        target = json.at(key).get<T>();
    }
}

template <typename T> void add_optional(Json& json, char const *key, std::optional<T> const& value) {
    if (value.has_value()) {
        json[key] = *value;
    }
}

void add_optional_string(Json& json, char const *key, std::optional<std::string> const& value) {
    if (value.has_value() && !value->empty()) {
        json[key] = *value;
    }
}

} // namespace

void to_json(Json& j, Address const& value) {
    j = Json{
        {"street_address", value.street_address},
        {"city",           value.city          },
        {"state",          value.state         },
        {"postal_code",    value.postal_code   },
        {"country",        value.country       }
    };
}

void from_json(Json const& j, Address& value) {
    j.at("street_address").get_to(value.street_address);
    j.at("city").get_to(value.city);
    j.at("state").get_to(value.state);
    j.at("postal_code").get_to(value.postal_code);
    j.at("country").get_to(value.country);
}

void to_json(Json& j, ContactInformation const& value) {
    j = Json{
        {"email_address", value.email_address},
        {"phone_number",  value.phone_number },
        {"address",       value.address      }
    };
}

void from_json(Json const& j, ContactInformation& value) {
    j.at("email_address").get_to(value.email_address);
    j.at("phone_number").get_to(value.phone_number);
    j.at("address").get_to(value.address);
}

void to_json(Json& j, IdentityInformation const& value) {
    j = Json{
        {"given_name",               value.given_name              },
        {"family_name",              value.family_name             },
        {"date_of_birth",            value.date_of_birth           },
        {"country_of_tax_residence", value.country_of_tax_residence},
        {"tax_id_type",              value.tax_id_type             },
        {"tax_id",                   value.tax_id                  },
        {"country_of_citizenship",   value.country_of_citizenship  }
    };
    add_optional_string(j, "middle_name", value.middle_name);
}

void from_json(Json const& j, IdentityInformation& value) {
    j.at("given_name").get_to(value.given_name);
    j.at("family_name").get_to(value.family_name);
    assign_optional(j, "middle_name", value.middle_name);
    j.at("date_of_birth").get_to(value.date_of_birth);
    j.at("country_of_tax_residence").get_to(value.country_of_tax_residence);
    j.at("tax_id_type").get_to(value.tax_id_type);
    j.at("tax_id").get_to(value.tax_id);
    j.at("country_of_citizenship").get_to(value.country_of_citizenship);
}

void to_json(Json& j, EmploymentInformation const& value) {
    j = Json{
        {"status", value.status}
    };
    add_optional_string(j, "employer_name", value.employer_name);
    add_optional_string(j, "employer_address", value.employer_address);
    add_optional_string(j, "position", value.position);
    add_optional_string(j, "source_of_funds", value.source_of_funds);
    add_optional_string(j, "annual_income_min", value.annual_income_min);
    add_optional_string(j, "annual_income_max", value.annual_income_max);
    add_optional_string(j, "liquid_net_worth_min", value.liquid_net_worth_min);
    add_optional_string(j, "liquid_net_worth_max", value.liquid_net_worth_max);
    add_optional_string(j, "total_net_worth_min", value.total_net_worth_min);
    add_optional_string(j, "total_net_worth_max", value.total_net_worth_max);
}

void from_json(Json const& j, EmploymentInformation& value) {
    j.at("status").get_to(value.status);
    assign_optional(j, "employer_name", value.employer_name);
    assign_optional(j, "employer_address", value.employer_address);
    assign_optional(j, "position", value.position);
    assign_optional(j, "source_of_funds", value.source_of_funds);
    assign_optional(j, "annual_income_min", value.annual_income_min);
    assign_optional(j, "annual_income_max", value.annual_income_max);
    assign_optional(j, "liquid_net_worth_min", value.liquid_net_worth_min);
    assign_optional(j, "liquid_net_worth_max", value.liquid_net_worth_max);
    assign_optional(j, "total_net_worth_min", value.total_net_worth_min);
    assign_optional(j, "total_net_worth_max", value.total_net_worth_max);
}

void to_json(Json& j, DisclosureInformation const& value) {
    j = Json{
        {"is_control_person",               value.is_control_person              },
        {"is_affiliated_exchange_or_finra", value.is_affiliated_exchange_or_finra},
        {"is_politically_exposed",          value.is_politically_exposed         },
        {"immediate_family_exposed",        value.immediate_family_exposed       }
    };
}

void from_json(Json const& j, DisclosureInformation& value) {
    j.at("is_control_person").get_to(value.is_control_person);
    j.at("is_affiliated_exchange_or_finra").get_to(value.is_affiliated_exchange_or_finra);
    j.at("is_politically_exposed").get_to(value.is_politically_exposed);
    j.at("immediate_family_exposed").get_to(value.immediate_family_exposed);
}

void to_json(Json& j, TrustedContactInformation const& value) {
    j = Json{
        {"given_name",  value.given_name },
        {"family_name", value.family_name}
    };
    add_optional_string(j, "email_address", value.email_address);
    add_optional_string(j, "phone_number", value.phone_number);
    if (value.address.has_value()) {
        j["address"] = *value.address;
    }
}

void from_json(Json const& j, TrustedContactInformation& value) {
    j.at("given_name").get_to(value.given_name);
    j.at("family_name").get_to(value.family_name);
    assign_optional(j, "email_address", value.email_address);
    assign_optional(j, "phone_number", value.phone_number);
    if (j.contains("address") && !j.at("address").is_null()) {
        value.address = j.at("address").get<Address>();
    }
}

void to_json(Json& j, AccountAgreement const& value) {
    j = Json{
        {"agreement",  value.agreement },
        {"signed_at",  value.signed_at },
        {"ip_address", value.ip_address}
    };
}

void from_json(Json const& j, AccountAgreement& value) {
    j.at("agreement").get_to(value.agreement);
    j.at("signed_at").get_to(value.signed_at);
    j.at("ip_address").get_to(value.ip_address);
}

void to_json(Json& j, AccountDocument const& value) {
    j = Json{
        {"id",            value.id           },
        {"document_type", value.document_type},
        {"status",        value.status       },
        {"created_at",    value.created_at   }
    };
    add_optional(j, "document_sub_type", value.document_sub_type);
    add_optional(j, "updated_at", value.updated_at);
    add_optional(j, "uploaded_at", value.uploaded_at);
    add_optional(j, "rejected_at", value.rejected_at);
    add_optional(j, "rejection_reason", value.rejection_reason);
}

void from_json(Json const& j, AccountDocument& value) {
    j.at("id").get_to(value.id);
    j.at("document_type").get_to(value.document_type);
    assign_optional(j, "document_sub_type", value.document_sub_type);
    j.at("status").get_to(value.status);
    j.at("created_at").get_to(value.created_at);
    assign_optional(j, "updated_at", value.updated_at);
    assign_optional(j, "uploaded_at", value.uploaded_at);
    assign_optional(j, "rejected_at", value.rejected_at);
    assign_optional(j, "rejection_reason", value.rejection_reason);
}

void to_json(Json& j, CreateAccountDocumentRequest const& value) {
    j = Json{
        {"document_type", value.document_type},
        {"content",       value.content      },
        {"mime_type",     value.mime_type    }
    };
    add_optional(j, "document_sub_type", value.document_sub_type);
    add_optional_string(j, "name", value.name);
}

void to_json(Json& j, CreateBrokerAccountRequest const& value) {
    j = Json{
        {"contact",     value.contact    },
        {"identity",    value.identity   },
        {"employment",  value.employment },
        {"disclosures", value.disclosures},
        {"agreements",  value.agreements },
        {"documents",   value.documents  }
    };
    if (value.trusted_contact.has_value()) {
        j["trusted_contact"] = *value.trusted_contact;
    }
}

void to_json(Json& j, UpdateBrokerAccountRequest const& value) {
    j = Json{};
    if (value.contact.has_value()) {
        j["contact"] = *value.contact;
    }
    if (value.identity.has_value()) {
        j["identity"] = *value.identity;
    }
    if (value.employment.has_value()) {
        j["employment"] = *value.employment;
    }
    if (value.disclosures.has_value()) {
        j["disclosures"] = *value.disclosures;
    }
    if (value.trusted_contact.has_value()) {
        j["trusted_contact"] = *value.trusted_contact;
    }
}

void from_json(Json const& j, BrokerAccount& value) {
    j.at("id").get_to(value.id);
    j.at("account_number").get_to(value.account_number);
    j.at("status").get_to(value.status);
    j.at("created_at").get_to(value.created_at);
    assign_optional(j, "updated_at", value.updated_at);
    assign_optional(j, "last_equity", value.last_equity);
    assign_optional(j, "cash", value.cash);
    j.at("contact").get_to(value.contact);
    j.at("identity").get_to(value.identity);
    j.at("employment").get_to(value.employment);
    j.at("disclosures").get_to(value.disclosures);
    if (j.contains("trusted_contact") && !j.at("trusted_contact").is_null()) {
        value.trusted_contact = j.at("trusted_contact").get<TrustedContactInformation>();
    }
    if (j.contains("agreements")) {
        value.agreements = j.at("agreements").get<std::vector<AccountAgreement>>();
    }
    if (j.contains("documents")) {
        value.documents = j.at("documents").get<std::vector<AccountDocument>>();
    }
}

QueryParams ListBrokerAccountsRequest::to_query_params() const {
    QueryParams params;
    if (status.has_value()) {
        params.emplace_back("status", *status);
    }
    if (entity_type.has_value()) {
        params.emplace_back("entity_type", *entity_type);
    }
    if (page_size.has_value()) {
        params.emplace_back("page_size", std::to_string(*page_size));
    }
    if (next_page_token.has_value()) {
        params.emplace_back("next_page_token", *next_page_token);
    }
    return params;
}

void from_json(Json const& j, BrokerAccountsPage& value) {
    if (j.contains("accounts")) {
        value.accounts = j.at("accounts").get<std::vector<BrokerAccount>>();
    } else if (j.is_array()) {
        value.accounts = j.get<std::vector<BrokerAccount>>();
    }
    assign_optional(j, "next_page_token", value.next_page_token);
}

void to_json(Json& j, CreateTransferRequest const& value) {
    j = Json{
        {"type",      value.type     },
        {"direction", value.direction},
        {"amount",    value.amount   }
    };
    add_optional_string(j, "timing", value.timing);
    add_optional_string(j, "relationship_id", value.relationship_id);
    add_optional_string(j, "transfer_id", value.transfer_id);
}

void from_json(Json const& j, Transfer& value) {
    j.at("id").get_to(value.id);
    j.at("account_id").get_to(value.account_id);
    j.at("status").get_to(value.status);
    j.at("type").get_to(value.type);
    j.at("direction").get_to(value.direction);
    j.at("amount").get_to(value.amount);
    assign_optional(j, "relationship_id", value.relationship_id);
    assign_optional(j, "reason", value.reason);
    j.at("created_at").get_to(value.created_at);
    assign_optional(j, "updated_at", value.updated_at);
    assign_optional(j, "completed_at", value.completed_at);
    assign_optional(j, "expires_at", value.expires_at);
}

QueryParams ListTransfersRequest::to_query_params() const {
    QueryParams params;
    if (status.has_value()) {
        params.emplace_back("status", *status);
    }
    if (direction.has_value()) {
        params.emplace_back("direction", *direction);
    }
    if (type.has_value()) {
        params.emplace_back("type", *type);
    }
    if (page_size.has_value()) {
        params.emplace_back("page_size", std::to_string(*page_size));
    }
    if (next_page_token.has_value()) {
        params.emplace_back("next_page_token", *next_page_token);
    }
    return params;
}

void from_json(Json const& j, TransfersPage& value) {
    if (j.contains("transfers")) {
        value.transfers = j.at("transfers").get<std::vector<Transfer>>();
    } else if (j.is_array()) {
        value.transfers = j.get<std::vector<Transfer>>();
    }
    assign_optional(j, "next_page_token", value.next_page_token);
}

void to_json(Json& j, CreateJournalRequest const& value) {
    j = Json{
        {"entry_type",  value.entry_type },
        {"description", value.description},
        {"amount",      value.amount     }
    };
    add_optional_string(j, "debit_account_id", value.debit_account_id);
    add_optional_string(j, "credit_account_id", value.credit_account_id);
}

void from_json(Json const& j, Journal& value) {
    j.at("id").get_to(value.id);
    j.at("status").get_to(value.status);
    j.at("entry_type").get_to(value.entry_type);
    j.at("description").get_to(value.description);
    j.at("amount").get_to(value.amount);
    j.at("created_at").get_to(value.created_at);
    assign_optional(j, "updated_at", value.updated_at);
    assign_optional(j, "debit_account_id", value.debit_account_id);
    assign_optional(j, "credit_account_id", value.credit_account_id);
    assign_optional(j, "reason", value.reason);
}

QueryParams ListJournalsRequest::to_query_params() const {
    QueryParams params;
    if (status.has_value()) {
        params.emplace_back("status", *status);
    }
    if (entry_type.has_value()) {
        params.emplace_back("entry_type", *entry_type);
    }
    if (page_size.has_value()) {
        params.emplace_back("page_size", std::to_string(*page_size));
    }
    if (next_page_token.has_value()) {
        params.emplace_back("next_page_token", *next_page_token);
    }
    return params;
}

void from_json(Json const& j, JournalsPage& value) {
    if (j.contains("journals")) {
        value.journals = j.at("journals").get<std::vector<Journal>>();
    } else if (j.is_array()) {
        value.journals = j.get<std::vector<Journal>>();
    }
    assign_optional(j, "next_page_token", value.next_page_token);
}

void to_json(Json& j, CreateAchRelationshipRequest const& value) {
    j = Json{
        {"bank_account_type", value.bank_account_type},
        {"routing_number",    value.routing_number   },
        {"account_number",    value.account_number   }
    };
    add_optional_string(j, "nickname", value.nickname);
    add_optional_string(j, "account_owner_name", value.account_owner_name);
}

void to_json(Json& j, CreateWireRelationshipRequest const& value) {
    j = Json{
        {"bank_account_type",   value.bank_account_type  },
        {"beneficiary_bank",    value.beneficiary_bank   },
        {"beneficiary_account", value.beneficiary_account}
    };
    add_optional_string(j, "nickname", value.nickname);
    add_optional_string(j, "account_owner_name", value.account_owner_name);
}

void from_json(Json const& j, BankRelationship& value) {
    j.at("id").get_to(value.id);
    j.at("account_id").get_to(value.account_id);
    j.at("status").get_to(value.status);
    j.at("bank_account_type").get_to(value.bank_account_type);
    assign_optional(j, "nickname", value.nickname);
    assign_optional(j, "bank_name", value.bank_name);
    assign_optional(j, "account_owner_name", value.account_owner_name);
    assign_optional(j, "created_at", value.created_at);
}

void from_json(Json const& j, BankRelationshipsPage& value) {
    if (j.contains("relationships")) {
        value.relationships = j.at("relationships").get<std::vector<BankRelationship>>();
    } else if (j.is_array()) {
        value.relationships = j.get<std::vector<BankRelationship>>();
    }
    assign_optional(j, "next_page_token", value.next_page_token);
}

void from_json(Json const& j, BrokerWatchlist& value) {
    j.at("id").get_to(value.id);
    j.at("name").get_to(value.name);
    j.at("account_id").get_to(value.account_id);
    j.at("created_at").get_to(value.created_at);
    j.at("updated_at").get_to(value.updated_at);
    if (j.contains("assets") && j.at("assets").is_array()) {
        value.assets = j.at("assets").get<std::vector<Asset>>();
    } else {
        value.assets.clear();
    }
}

void to_json(Json& j, CreateBrokerWatchlistRequest const& value) {
    j = Json{{"name", value.name}, {"symbols", value.symbols}};
}

void to_json(Json& j, UpdateBrokerWatchlistRequest const& value) {
    j = Json{};
    add_optional_string(j, "name", value.name);
    if (value.symbols.has_value()) {
        j["symbols"] = *value.symbols;
    }
}

void to_json(Json& j, RebalancingWeight const& value) {
    j = Json{{"type", value.type}, {"percent", value.percent}};
    add_optional_string(j, "symbol", value.symbol);
}

void from_json(Json const& j, RebalancingWeight& value) {
    j.at("type").get_to(value.type);
    assign_optional(j, "symbol", value.symbol);
    j.at("percent").get_to(value.percent);
}

void to_json(Json& j, RebalancingCondition const& value) {
    j = Json{
        {"type", value.type},
        {"sub_type", value.sub_type},
    };
    if (value.percent.has_value()) {
        j["percent"] = *value.percent;
    }
    add_optional_string(j, "day", value.day);
}

void from_json(Json const& j, RebalancingCondition& value) {
    j.at("type").get_to(value.type);
    j.at("sub_type").get_to(value.sub_type);
    assign_optional(j, "percent", value.percent);
    assign_optional(j, "day", value.day);
}

void from_json(Json const& j, RebalancingPortfolio& value) {
    j.at("id").get_to(value.id);
    j.at("name").get_to(value.name);
    j.at("description").get_to(value.description);
    j.at("status").get_to(value.status);
    j.at("cooldown_days").get_to(value.cooldown_days);
    j.at("created_at").get_to(value.created_at);
    j.at("updated_at").get_to(value.updated_at);
    if (j.contains("weights")) {
        value.weights = j.at("weights").get<std::vector<RebalancingWeight>>();
    }
    if (j.contains("rebalance_conditions") && !j.at("rebalance_conditions").is_null()) {
        value.rebalance_conditions =
            j.at("rebalance_conditions").get<std::vector<RebalancingCondition>>();
    }
}

void to_json(Json& j, CreateRebalancingPortfolioRequest const& value) {
    j = Json{
        {"name", value.name},
        {"description", value.description},
        {"weights", value.weights},
        {"cooldown_days", value.cooldown_days},
    };
    if (value.rebalance_conditions.has_value()) {
        j["rebalance_conditions"] = *value.rebalance_conditions;
    }
}

void to_json(Json& j, UpdateRebalancingPortfolioRequest const& value) {
    j = Json{};
    add_optional_string(j, "name", value.name);
    add_optional_string(j, "description", value.description);
    if (value.weights.has_value()) {
        j["weights"] = *value.weights;
    }
    if (value.cooldown_days.has_value()) {
        j["cooldown_days"] = *value.cooldown_days;
    }
    if (value.rebalance_conditions.has_value()) {
        j["rebalance_conditions"] = *value.rebalance_conditions;
    }
}

QueryParams ListRebalancingPortfoliosRequest::to_query_params() const {
    QueryParams params;
    if (name.has_value()) {
        params.emplace_back("name", *name);
    }
    if (description.has_value()) {
        params.emplace_back("description", *description);
    }
    if (symbol.has_value()) {
        params.emplace_back("symbol", *symbol);
    }
    if (portfolio_id.has_value()) {
        params.emplace_back("portfolio_id", *portfolio_id);
    }
    if (status.has_value()) {
        params.emplace_back("status", *status);
    }
    return params;
}

void from_json(Json const& j, RebalancingSubscription& value) {
    j.at("id").get_to(value.id);
    j.at("account_id").get_to(value.account_id);
    j.at("portfolio_id").get_to(value.portfolio_id);
    j.at("created_at").get_to(value.created_at);
    assign_optional(j, "last_rebalanced_at", value.last_rebalanced_at);
}

void to_json(Json& j, CreateRebalancingSubscriptionRequest const& value) {
    j = Json{{"account_id", value.account_id}, {"portfolio_id", value.portfolio_id}};
}

QueryParams ListRebalancingSubscriptionsRequest::to_query_params() const {
    QueryParams params;
    if (account_id.has_value()) {
        params.emplace_back("account_id", *account_id);
    }
    if (portfolio_id.has_value()) {
        params.emplace_back("portfolio_id", *portfolio_id);
    }
    if (limit.has_value()) {
        params.emplace_back("limit", std::to_string(*limit));
    }
    if (page_token.has_value()) {
        params.emplace_back("page_token", *page_token);
    }
    return params;
}

void from_json(Json const& j, RebalancingSubscriptionsPage& value) {
    if (j.contains("subscriptions")) {
        value.subscriptions = j.at("subscriptions").get<std::vector<RebalancingSubscription>>();
    } else if (j.is_array()) {
        value.subscriptions = j.get<std::vector<RebalancingSubscription>>();
    }
    assign_optional(j, "next_page_token", value.next_page_token);
}

void from_json(Json const& j, ManagedPortfolioHistory& value) {
    if (j.contains("timestamp")) {
        value.timestamp = j.at("timestamp").get<std::vector<int64_t>>();
    }
    if (j.contains("equity")) {
        value.equity = j.at("equity").get<std::vector<double>>();
    }
    if (j.contains("profit_loss")) {
        value.profit_loss = j.at("profit_loss").get<std::vector<double>>();
    }
    value.profit_loss_pct.clear();
    if (j.contains("profit_loss_pct") && j.at("profit_loss_pct").is_array()) {
        for (auto const& entry : j.at("profit_loss_pct")) {
            if (entry.is_null()) {
                value.profit_loss_pct.emplace_back(std::nullopt);
            } else {
                value.profit_loss_pct.emplace_back(entry.get<double>());
            }
        }
    }
    assign_optional(j, "base_value", value.base_value);
    if (j.contains("timeframe")) {
        value.timeframe = j.at("timeframe").get<std::string>();
    }
    value.cashflow.clear();
    if (j.contains("cashflow") && j.at("cashflow").is_object()) {
        for (auto const& [key, val] : j.at("cashflow").items()) {
            value.cashflow[key] = val.get<std::vector<double>>();
        }
    }
}

QueryParams ManagedPortfolioHistoryRequest::to_query_params() const {
    QueryParams params;
    if (period.has_value()) {
        params.emplace_back("period", *period);
    }
    if (timeframe.has_value()) {
        params.emplace_back("timeframe", *timeframe);
    }
    if (intraday_reporting.has_value()) {
        params.emplace_back("intraday_reporting", *intraday_reporting);
    }
    if (start.has_value()) {
        params.emplace_back("start", *start);
    }
    if (pnl_reset.has_value()) {
        params.emplace_back("pnl_reset", *pnl_reset);
    }
    if (end.has_value()) {
        params.emplace_back("end", *end);
    }
    if (date_end.has_value()) {
        params.emplace_back("date_end", *date_end);
    }
    if (extended_hours.has_value()) {
        params.emplace_back("extended_hours", *extended_hours ? "true" : "false");
    }
    if (cashflow_types.has_value()) {
        params.emplace_back("cashflow_types", *cashflow_types);
    }
    return params;
}

} // namespace alpaca
