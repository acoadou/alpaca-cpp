#include "alpaca/models/Watchlist.hpp"

namespace alpaca {

void from_json(const Json& j, Watchlist& watchlist) {
  watchlist.id = j.value("id", std::string{});
  watchlist.name = j.value("name", std::string{});
  watchlist.account_id = j.value("account_id", std::string{});
  watchlist.created_at = j.value("created_at", std::string{});
  watchlist.updated_at = j.value("updated_at", std::string{});
  if (j.contains("assets") && !j.at("assets").is_null()) {
    watchlist.assets = j.at("assets").get<std::vector<Asset>>();
  } else {
    watchlist.assets.clear();
  }
}

void to_json(Json& j, const CreateWatchlistRequest& request) {
  j = Json{{"name", request.name}, {"symbols", request.symbols}};
}

void to_json(Json& j, const UpdateWatchlistRequest& request) {
  j = Json{};
  if (request.name.has_value()) {
    j["name"] = *request.name;
  }
  if (request.symbols.has_value()) {
    j["symbols"] = *request.symbols;
  }
}

}  // namespace alpaca
