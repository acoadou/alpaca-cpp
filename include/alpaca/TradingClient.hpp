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
#include "alpaca/models/Option.hpp"
#include "alpaca/models/Order.hpp"
#include "alpaca/models/PortfolioHistory.hpp"
#include "alpaca/models/Position.hpp"
#include "alpaca/models/Watchlist.hpp"

namespace alpaca {

/// High-level trading surface exposing account/order/watchlist operations.
class TradingClient {
  public:
    TradingClient(Configuration const& config, HttpClientPtr http_client = nullptr,
                  RestClient::Options options = RestClient::default_options());
    TradingClient(Configuration const& config, RestClient::Options options);
    TradingClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                  HttpClientPtr http_client = nullptr, RestClient::Options options = RestClient::default_options());
    TradingClient(Environment const& environment, std::string api_key_id, std::string api_secret_key,
                  RestClient::Options options);

    [[nodiscard]] Account get_account();
    [[nodiscard]] AccountConfiguration get_account_configuration();
    [[nodiscard]] AccountConfiguration update_account_configuration(AccountConfigurationUpdateRequest const& request);

    [[nodiscard]] std::vector<Position> list_positions();
    [[nodiscard]] Position get_position(std::string const& symbol);
    [[nodiscard]] Position close_position(std::string const& symbol, ClosePositionRequest const& request = {});
    [[nodiscard]] std::vector<ClosePositionResponse> close_all_positions(CloseAllPositionsRequest const& request = {});

    [[nodiscard]] std::vector<OptionPosition> list_option_positions();
    [[nodiscard]] OptionPosition get_option_position(std::string const& symbol);
    [[nodiscard]] OptionPosition close_option_position(std::string const& symbol,
                                                       CloseOptionPositionRequest const& request = {});
    void exercise_options_position(std::string const& symbol_or_contract_id);

    [[nodiscard]] std::vector<Order> list_orders(ListOrdersRequest const& request = {});
    [[nodiscard]] Order get_order(std::string const& order_id);
    [[nodiscard]] Order get_order_by_client_order_id(std::string const& client_order_id);
    void cancel_order(std::string const& order_id);
    [[nodiscard]] std::vector<CancelledOrderId> cancel_all_orders();
    [[nodiscard]] Order submit_order(NewOrderRequest const& request);
    [[nodiscard]] Order replace_order(std::string const& order_id, ReplaceOrderRequest const& request);

    [[nodiscard]] std::vector<OptionOrder> list_option_orders(ListOptionOrdersRequest const& request = {});
    [[nodiscard]] OptionOrder get_option_order(std::string const& order_id);
    [[nodiscard]] OptionOrder get_option_order_by_client_order_id(std::string const& client_order_id);
    void cancel_option_order(std::string const& order_id);
    [[nodiscard]] std::vector<OptionCancelledOrderId> cancel_all_option_orders();
    [[nodiscard]] OptionOrder submit_option_order(NewOptionOrderRequest const& request);
    [[nodiscard]] OptionOrder replace_option_order(std::string const& order_id,
                                                   ReplaceOptionOrderRequest const& request);

    [[nodiscard]] std::vector<CryptoOrder> list_crypto_orders(ListCryptoOrdersRequest request = {});
    [[nodiscard]] CryptoOrder get_crypto_order(std::string const& order_id);
    [[nodiscard]] CryptoOrder get_crypto_order_by_client_order_id(std::string const& client_order_id);
    void cancel_crypto_order(std::string const& order_id);
    [[nodiscard]] std::vector<CryptoCancelledOrderId> cancel_all_crypto_orders();
    [[nodiscard]] CryptoOrder submit_crypto_order(NewCryptoOrderRequest const& request);
    [[nodiscard]] CryptoOrder replace_crypto_order(std::string const& order_id,
                                                   ReplaceCryptoOrderRequest const& request);

    [[nodiscard]] std::vector<OtcOrder> list_otc_orders(ListOtcOrdersRequest request = {});
    [[nodiscard]] OtcOrder get_otc_order(std::string const& order_id);
    [[nodiscard]] OtcOrder get_otc_order_by_client_order_id(std::string const& client_order_id);
    void cancel_otc_order(std::string const& order_id);
    [[nodiscard]] std::vector<OtcCancelledOrderId> cancel_all_otc_orders();
    [[nodiscard]] OtcOrder submit_otc_order(NewOtcOrderRequest const& request);
    [[nodiscard]] OtcOrder replace_otc_order(std::string const& order_id, ReplaceOtcOrderRequest const& request);

    [[nodiscard]] OptionContractsResponse list_option_contracts(ListOptionContractsRequest const& request = {});
    [[nodiscard]] OptionContract get_option_contract(std::string const& symbol);
    [[nodiscard]] OptionAnalyticsResponse list_option_analytics(ListOptionAnalyticsRequest const& request = {});
    [[nodiscard]] OptionAnalytics get_option_analytics(std::string const& symbol);

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
    [[nodiscard]] Watchlist add_asset_to_watchlist_by_name(std::string const& name, std::string const& symbol);
    [[nodiscard]] Watchlist remove_asset_from_watchlist(std::string const& id, std::string const& symbol);
    [[nodiscard]] Watchlist remove_asset_from_watchlist_by_name(std::string const& name, std::string const& symbol);
    void delete_watchlist(std::string const& id);
    void delete_watchlist_by_name(std::string const& name);

  private:
    RestClient rest_client_;
};

} // namespace alpaca
