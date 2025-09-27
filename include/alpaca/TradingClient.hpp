#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "alpaca/Configuration.hpp"
#include "alpaca/HttpClient.hpp"
#include "alpaca/RestClient.hpp"
#include "alpaca/models/Account.hpp"
#include "alpaca/models/AccountActivity.hpp"
#include "alpaca/models/AccountConfiguration.hpp"
#include "alpaca/models/Asset.hpp"
#include "alpaca/models/CalendarDay.hpp"
#include "alpaca/models/Clock.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/PortfolioHistory.hpp"
#include "alpaca/models/Position.hpp"
#include "alpaca/models/Watchlist.hpp"

namespace alpaca {

/// High-level trading surface exposing account/order/watchlist operations.
class TradingClient {
  public:
    TradingClient(Configuration const& config, HttpClientPtr http_client = nullptr);
    TradingClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                  HttpClientPtr http_client = nullptr);

    [[nodiscard]] Account get_account();
    [[nodiscard]] AccountConfiguration get_account_configuration();
    [[nodiscard]] AccountConfiguration update_account_configuration(AccountConfigurationUpdateRequest const& request);

    [[nodiscard]] std::vector<Position> list_positions();
    [[nodiscard]] Position get_position(std::string const& symbol);
    [[nodiscard]] Position close_position(std::string const& symbol, ClosePositionRequest const& request = {});

    [[nodiscard]] std::vector<Order> list_orders(ListOrdersRequest const& request = {});
    [[nodiscard]] Order get_order(std::string const& order_id);
    [[nodiscard]] Order get_order_by_client_order_id(std::string const& client_order_id);
    void cancel_order(std::string const& order_id);
    [[nodiscard]] std::vector<CancelledOrderId> cancel_all_orders();
    [[nodiscard]] Order submit_order(NewOrderRequest const& request);
    [[nodiscard]] Order replace_order(std::string const& order_id, ReplaceOrderRequest const& request);

    [[nodiscard]] Clock get_clock();
    [[nodiscard]] std::vector<CalendarDay> get_calendar(CalendarRequest const& request = {});

    [[nodiscard]] std::vector<Asset> list_assets(ListAssetsRequest const& request = {});
    [[nodiscard]] Asset get_asset(std::string const& symbol);

    [[nodiscard]] std::vector<AccountActivity> get_account_activities(AccountActivitiesRequest const& request = {});
    [[nodiscard]] PortfolioHistory get_portfolio_history(PortfolioHistoryRequest const& request = {});

    [[nodiscard]] std::vector<Watchlist> list_watchlists();
    [[nodiscard]] Watchlist get_watchlist(std::string const& id);
    [[nodiscard]] Watchlist get_watchlist_by_name(std::string const& name);
    [[nodiscard]] Watchlist create_watchlist(CreateWatchlistRequest const& request);
    [[nodiscard]] Watchlist update_watchlist(std::string const& id, UpdateWatchlistRequest const& request);
    [[nodiscard]] Watchlist add_asset_to_watchlist(std::string const& id, std::string const& symbol);
    [[nodiscard]] Watchlist remove_asset_from_watchlist(std::string const& id, std::string const& symbol);
    void delete_watchlist(std::string const& id);

  private:
    RestClient rest_client_;
};

} // namespace alpaca
