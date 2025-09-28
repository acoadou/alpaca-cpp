#include "alpaca/Money.hpp"

#include "alpaca/Exceptions.hpp"

#include <limits>

#include <gtest/gtest.h>

namespace alpaca {
namespace {

TEST(MoneyTest, ParsesTrimmedStringValues) {
    Money amount{"  15.500100  "};
    EXPECT_EQ(amount.raw(), 15 * Money::kScale + 500100);
}

TEST(MoneyTest, ParsesMaximumAndMinimumValues) {
    Money const max_text{"9223372036854.775807"};
    EXPECT_EQ(max_text.raw(), std::numeric_limits<std::int64_t>::max());

    Money const min_text{"-9223372036854.775808"};
    EXPECT_EQ(min_text.raw(), std::numeric_limits<std::int64_t>::min());
}

TEST(MoneyTest, RejectsInvalidTextRepresentations) {
    EXPECT_THROW(Money{"-"}, InvalidArgumentException);
    EXPECT_THROW(Money{"12abc"}, InvalidArgumentException);
    EXPECT_THROW(Money{"1.2345678"}, InvalidArgumentException);
    EXPECT_THROW(Money{"1."}, InvalidArgumentException);
}

TEST(MoneyTest, RejectsOutOfRangeTextValues) {
    EXPECT_THROW(Money{"9223372036854.775808"}, InvalidArgumentException);
    EXPECT_THROW(Money{"-9223372036854.775809"}, InvalidArgumentException);
}

TEST(MoneyTest, RejectsNonFiniteDoubles) {
    EXPECT_THROW(Money{std::numeric_limits<double>::infinity()}, InvalidArgumentException);
    EXPECT_THROW(Money{-std::numeric_limits<double>::infinity()}, InvalidArgumentException);
    EXPECT_THROW(Money{std::numeric_limits<double>::quiet_NaN()}, InvalidArgumentException);
}

TEST(MoneyTest, RejectsOutOfRangeDoubles) {
    EXPECT_THROW(Money{1e20}, InvalidArgumentException);
    EXPECT_THROW(Money{-1e20}, InvalidArgumentException);
}

TEST(MoneyTest, AcceptsFractionOnlyValues) {
    Money amount{".250001"};
    EXPECT_EQ(amount.raw(), 250001);
}

} // namespace
} // namespace alpaca
