#include "alpaca/RestClient.hpp"

#include <cctype>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <stdexcept>
#include <string_view>

#include "alpaca/ApiException.hpp"
#include "alpaca/version.hpp"

namespace alpaca {
namespace {
bool is_unreserved(unsigned char c) {
  return std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.' || c == '~';
}

std::string url_encode(std::string_view value) {
  std::ostringstream encoded;
  encoded.fill('0');
  encoded << std::hex << std::uppercase;
  for (unsigned char c : value) {
    if (is_unreserved(c)) {
      encoded << static_cast<char>(c);
    } else {
      encoded << '%' << std::setw(2) << static_cast<int>(c);
    }
  }
  return encoded.str();
}
}  // namespace

RestClient::RestClient(Configuration config, HttpClientPtr http_client, std::string base_url)
    : config_(std::move(config)), http_client_(std::move(http_client)), base_url_(std::move(base_url)) {
  if (!http_client_) {
    throw std::invalid_argument("RestClient requires a non-null HttpClient instance");
  }
  if (!config_.has_credentials()) {
    throw std::invalid_argument("Configuration must contain API credentials");
  }
}

std::string RestClient::build_url(const std::string& base, const std::string& path, const QueryParams& params) {
  std::ostringstream url;
  url << base;
  if (!path.empty() && path.front() != '/') {
    url << '/';
  }
  url << path;
  const std::string query = encode_query(params);
  if (!query.empty()) {
    url << '?' << query;
  }
  return url.str();
}

std::string RestClient::encode_query(const QueryParams& params) {
  if (params.empty()) {
    return {};
  }
  std::ostringstream query;
  bool first = true;
  for (const auto& [key, value] : params) {
    if (!first) {
      query << '&';
    }
    first = false;
    query << url_encode(key) << '=' << url_encode(value);
  }
  return query.str();
}

std::optional<Json> RestClient::perform_request(HttpMethod method, const std::string& path, const QueryParams& params,
                                                std::optional<std::string> payload) {
  std::string url = build_url(base_url_, path, params);

  HttpRequest request;
  request.method = method;
  request.url = std::move(url);
  request.timeout = config_.timeout;
  request.headers = config_.default_headers;

  const bool has_key_secret = !config_.api_key_id.empty() && !config_.api_secret_key.empty();
  if (has_key_secret) {
    request.headers["APCA-API-KEY-ID"] = config_.api_key_id;
    request.headers["APCA-API-SECRET-KEY"] = config_.api_secret_key;
  } else {
    const bool has_authorization_header =
        request.headers.find("Authorization") != request.headers.end();
    if (!has_authorization_header && config_.bearer_token.has_value() && !config_.bearer_token->empty()) {
      request.headers["Authorization"] = std::string("Bearer ") + *config_.bearer_token;
    }
  }
  request.headers["Accept"] = "application/json";
  if (request.headers.find("User-Agent") == request.headers.end()) {
    request.headers["User-Agent"] = std::string("alpaca-cpp/") + std::string(kVersion);
  }
  request.verify_peer = config_.verify_ssl;
  request.verify_host = config_.verify_hostname;
  request.ca_bundle_path = config_.ca_bundle_path;
  request.ca_bundle_dir = config_.ca_bundle_dir;

  if (payload.has_value()) {
    request.body = std::move(*payload);
    request.headers["Content-Type"] = "application/json";
  }

  HttpResponse response = http_client_->send(request);
  if (response.status_code >= 400) {
    std::string message = "HTTP " + std::to_string(response.status_code);
    try {
      Json error_body = Json::parse(response.body);
      if (error_body.contains("message")) {
        message = error_body.at("message").get<std::string>();
      }
    } catch (const std::exception&) {
      // Ignore parse errors and retain the default message.
    }
    throw ApiException(response.status_code, message, response.body, std::move(response.headers));
  }

  if (response.body.empty()) {
    return std::nullopt;
  }

  return Json::parse(response.body);
}

}  // namespace alpaca

