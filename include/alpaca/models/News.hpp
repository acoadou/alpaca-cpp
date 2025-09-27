#pragma once

#include <optional>
#include <string>
#include <vector>

#include "alpaca/Json.hpp"
#include "alpaca/models/Common.hpp"

namespace alpaca {

/// Represents an image associated with an Alpaca news article.
struct NewsImage {
    std::string url;
    std::optional<std::string> caption{};
    std::optional<std::string> size{};
};

void from_json(Json const& j, NewsImage& image);

/// Represents a single Alpaca news article with strongly typed fields.
struct NewsArticle {
    std::string id;
    std::string headline;
    std::optional<std::string> author{};
    std::optional<std::string> summary{};
    std::optional<std::string> content{};
    std::string url;
    std::string source;
    std::vector<std::string> symbols{};
    std::vector<NewsImage> images{};
    std::optional<Timestamp> created_at{};
    std::optional<Timestamp> updated_at{};
    Json raw{};
};

void from_json(Json const& j, NewsArticle& article);

/// Envelope returned by the Alpaca news endpoint.
struct NewsResponse {
    std::vector<NewsArticle> news{};
    std::optional<std::string> next_page_token{};
};

void from_json(Json const& j, NewsResponse& response);

} // namespace alpaca
