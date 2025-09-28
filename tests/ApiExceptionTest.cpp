#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "alpaca/Exceptions.hpp"
#include "alpaca/Exceptions.hpp"
#include "alpaca/HttpHeaders.hpp"

namespace {

TEST(ExceptionTest, ParsesDeltaSecondsRetryAfter) {
    alpaca::HttpHeaders headers;
    headers.append("Retry-After", "5");

    alpaca::Exception exception(429, "rate limit", "{}", headers);

    auto retry_after = exception.retry_after();
    ASSERT_TRUE(retry_after.has_value());
    EXPECT_EQ(*retry_after, std::chrono::seconds{5});
}

TEST(ExceptionTest, ParsesHttpDateRetryAfter) {
    auto const target = std::chrono::system_clock::now() + std::chrono::seconds{10};
    std::time_t const target_time = std::chrono::system_clock::to_time_t(target);

    std::tm utc_tm{};
#if defined(_WIN32)
    gmtime_s(&utc_tm, &target_time);
#else
    gmtime_r(&target_time, &utc_tm);
#endif

    std::ostringstream header_value;
    header_value << std::put_time(&utc_tm, "%a, %d %b %Y %H:%M:%S GMT");

    alpaca::HttpHeaders headers;
    headers.append("Retry-After", header_value.str());

    alpaca::Exception exception(429, "rate limit", "{}", headers);

    auto retry_after = exception.retry_after();
    ASSERT_TRUE(retry_after.has_value());
    EXPECT_GE(retry_after->count(), 8);
    EXPECT_LE(retry_after->count(), 10);
}

TEST(ExceptionTest, ReportsHttpContextMetadata) {
    alpaca::HttpHeaders headers;
    headers.append("Retry-After", "5");
    headers.append("X-Test", "value");

    alpaca::Exception exception(503, "server", "{}", headers);

    EXPECT_TRUE(exception.has_http_context());
    EXPECT_EQ(exception.status_code(), 503);
    EXPECT_TRUE(exception.retry_after().has_value());
    auto const& metadata = exception.metadata();
    auto status_entry = metadata.find("status_code");
    ASSERT_NE(status_entry, metadata.end());
    EXPECT_EQ(status_entry->second, "503");

    auto header_entry = metadata.find("header_count");
    ASSERT_NE(header_entry, metadata.end());
    EXPECT_EQ(header_entry->second, "2");
}

TEST(ExceptionTest, NonHttpExceptionHasNoContext) {
    alpaca::InvalidArgumentException exception("field", "bad value");

    EXPECT_FALSE(exception.has_http_context());
    EXPECT_EQ(exception.status_code(), 0);
    EXPECT_FALSE(exception.retry_after().has_value());
    EXPECT_EQ(exception.headers().size(), 0U);
}

} // namespace
