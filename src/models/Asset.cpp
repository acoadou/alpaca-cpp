#include "alpaca/models/Asset.hpp"

namespace alpaca {

void from_json(Json const& j, Asset& asset) {
    j.at("id").get_to(asset.id);
    asset.symbol = j.value("symbol", std::string{});
    asset.name = j.value("name", std::string{});
    asset.exchange = j.value("exchange", std::string{});
    if (j.contains("class") && !j.at("class").is_null()) {
        asset.asset_class = asset_class_from_string(j.at("class").get<std::string>());
    }
    if (j.contains("status") && !j.at("status").is_null()) {
        asset.status = asset_status_from_string(j.at("status").get<std::string>());
    }
    asset.tradable = j.value("tradable", false);
    asset.marginable = j.value("marginable", false);
    asset.shortable = j.value("shortable", false);
    asset.easy_to_borrow = j.value("easy_to_borrow", false);
    asset.fractionable = j.value("fractionable", false);
    asset.maintenance_margin_requirement = j.value("maintenance_margin_requirement", std::string{});
    asset.margin_requirement_long = j.value("margin_requirement_long", std::string{});
    asset.margin_requirement_short = j.value("margin_requirement_short", std::string{});
    if (j.contains("min_order_size") && !j.at("min_order_size").is_null()) {
        asset.min_order_size = j.at("min_order_size").get<std::string>();
    } else {
        asset.min_order_size.reset();
    }
    if (j.contains("min_trade_increment") && !j.at("min_trade_increment").is_null()) {
        asset.min_trade_increment = j.at("min_trade_increment").get<std::string>();
    } else {
        asset.min_trade_increment.reset();
    }
    if (j.contains("price_increment") && !j.at("price_increment").is_null()) {
        asset.price_increment = j.at("price_increment").get<std::string>();
    } else {
        asset.price_increment.reset();
    }
}

QueryParams ListAssetsRequest::to_query_params() const {
    QueryParams params;
    if (status.has_value()) {
        params.emplace_back("status", to_string(*status));
    }
    if (asset_class.has_value()) {
        params.emplace_back("asset_class", to_string(*asset_class));
    }
    if (exchange.has_value()) {
        params.emplace_back("exchange", *exchange);
    }
    if (!symbols.empty()) {
        params.emplace_back("symbols", join_csv(symbols));
    }
    return params;
}

} // namespace alpaca
