#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include "alpaca/ApiException.hpp"

namespace {

TEST(ApiExceptionTest, ParsesDeltaSecondsRetryAfter) {
    std::unordered_map<std::string, std::string> headers{
        {"Retry-After", "5"}
    };

    alpaca::ApiException exception(429, "rate limit", "{}", headers);

    auto retry_after = exception.retry_after();
    ASSERT_TRUE(retry_after.has_value());
    EXPECT_EQ(*retry_after, std::chrono::seconds{5});
}

TEST(ApiExceptionTest, ParsesHttpDateRetryAfter) {
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

    std::unordered_map<std::string, std::string> headers{
        {"Retry-After", header_value.str()}
    };

    alpaca::ApiException exception(429, "rate limit", "{}", headers);

    auto retry_after = exception.retry_after();
    ASSERT_TRUE(retry_after.has_value());
    EXPECT_GE(retry_after->count(), 8);
    EXPECT_LE(retry_after->count(), 10);
}

} // namespace
