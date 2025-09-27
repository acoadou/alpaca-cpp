#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Represents a tradable Alpaca asset.
struct Asset {
    std::string id;
    std::string symbol;
    std::string name;
    std::string exchange;
    AssetClass asset_class{AssetClass::US_EQUITY};
    AssetStatus status{AssetStatus::ACTIVE};
    bool tradable{false};
    bool marginable{false};
    bool shortable{false};
    bool easy_to_borrow{false};
    bool fractionable{false};
    std::string maintenance_margin_requirement;
};

void from_json(Json const& j, Asset& asset);

/// Request parameters accepted by the list assets endpoint.
struct ListAssetsRequest {
    std::optional<AssetStatus> status{};
    std::optional<AssetClass> asset_class{};
    std::optional<std::string> exchange{};
    std::vector<std::string> symbols{};

    [[nodiscard]] QueryParams to_query_params() const;
};

} // namespace alpaca
