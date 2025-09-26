#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/models/Asset.hpp"

namespace alpaca {

/// Represents an Alpaca watchlist containing tracked assets.
struct Watchlist {
  std::string id;
  std::string name;
  std::string account_id;
  std::string created_at;
  std::string updated_at;
  std::vector<Asset> assets;
};

void from_json(const Json& j, Watchlist& watchlist);

/// Request payload used to create a new watchlist.
struct CreateWatchlistRequest {
  std::string name;
  std::vector<std::string> symbols;
};

/// Request payload used to update an existing watchlist.
struct UpdateWatchlistRequest {
  std::optional<std::string> name;
  std::optional<std::vector<std::string>> symbols;
};

void to_json(Json& j, const CreateWatchlistRequest& request);
void to_json(Json& j, const UpdateWatchlistRequest& request);

}  // namespace alpaca
