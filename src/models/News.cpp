#include "alpaca/models/News.hpp"

#include <cstdint>

namespace alpaca {
namespace {
std::optional<std::string> optional_string(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    return j.at(key).get<std::string>();
}

std::optional<std::string> optional_string_from_any(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    auto const& value = j.at(key);
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_number_integer()) {
        return std::to_string(value.get<std::int64_t>());
    }
    if (value.is_number_unsigned()) {
        return std::to_string(value.get<std::uint64_t>());
    }
    if (value.is_number_float()) {
        return std::to_string(value.get<double>());
    }
    return value.dump();
}

std::optional<Timestamp> optional_timestamp(Json const& j, char const *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::nullopt;
    }
    return parse_timestamp(j.at(key).get<std::string>());
}
} // namespace

void from_json(Json const& j, NewsImage& image) {
    j.at("url").get_to(image.url);
    image.caption = optional_string(j, "caption");
    image.size = optional_string(j, "size");
}

void from_json(Json const& j, NewsArticle& article) {
    article.raw = j;
    if (j.contains("id") && !j.at("id").is_null()) {
        auto const id_value = optional_string_from_any(j, "id");
        if (id_value.has_value()) {
            article.id = *id_value;
        }
    }
    if (article.id.empty()) {
        article.id = "";
    }
    if (j.contains("headline")) {
        article.headline = j.at("headline").get<std::string>();
    }
    article.author = optional_string_from_any(j, "author");
    article.summary = optional_string_from_any(j, "summary");
    article.content = optional_string_from_any(j, "content");
    if (j.contains("url")) {
        article.url = j.at("url").get<std::string>();
    }
    if (j.contains("source")) {
        article.source = j.at("source").get<std::string>();
    }
    if (j.contains("symbols") && j.at("symbols").is_array()) {
        article.symbols = j.at("symbols").get<std::vector<std::string>>();
    } else {
        article.symbols.clear();
    }
    if (j.contains("images") && j.at("images").is_array()) {
        article.images = j.at("images").get<std::vector<NewsImage>>();
    } else {
        article.images.clear();
    }
    article.created_at = optional_timestamp(j, "created_at");
    article.updated_at = optional_timestamp(j, "updated_at");
}

void from_json(Json const& j, NewsResponse& response) {
    if (j.contains("news") && j.at("news").is_array()) {
        response.news = j.at("news").get<std::vector<NewsArticle>>();
    } else {
        response.news.clear();
    }
    if (j.contains("next_page_token") && !j.at("next_page_token").is_null()) {
        response.next_page_token = j.at("next_page_token").get<std::string>();
    } else {
        response.next_page_token = std::nullopt;
    }
}

} // namespace alpaca
