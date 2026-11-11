#include "core/canonical_integer.h"

#include <gtest/gtest.h>

#include <limits>

using namespace economy;

TEST(CanonicalIntegerTest, RoundTripsTypedValuesAtBounds) {
    EXPECT_EQ(canonicalDecimal(parseCanonicalId<ActorTag>("1")), "1");
    EXPECT_EQ(canonicalDecimal(parseCanonicalId<CommandTag>("9223372036854775807")),
              "9223372036854775807");
    EXPECT_EQ(canonicalDecimal(parseCanonicalMoney("0")), "0");
    EXPECT_EQ(canonicalDecimal(parseCanonicalMoney("9223372036854775807")),
              "9223372036854775807");
    EXPECT_EQ(canonicalDecimal(parseCanonicalQuantity("12")), "12");
    EXPECT_EQ(canonicalDecimal(parseCanonicalSigned("-9223372036854775808")),
              "-9223372036854775808");
    EXPECT_EQ(canonicalDecimal(parseCanonicalSigned("9223372036854775807")),
              "9223372036854775807");
    EXPECT_EQ(canonicalDecimal(parseCanonicalSigned("0")), "0");
}

TEST(CanonicalIntegerTest, RejectsNoncanonicalGrammar) {
    for (const std::string input : {"", "+1", "-0", "01", "00", "1.0", "1e2", " 1", "1 ",
                                    "１２", "--1", "-01", "- 1"}) {
        EXPECT_THROW(parseCanonicalSigned(input), std::invalid_argument) << input;
    }
    EXPECT_THROW(parseCanonicalPositive("0"), std::invalid_argument);
    EXPECT_THROW(parseCanonicalMoney("-1"), std::invalid_argument);
    EXPECT_THROW(parseCanonicalQuantity("+1"), std::invalid_argument);
}

TEST(CanonicalIntegerTest, RejectsOutOfRangeValues) {
    EXPECT_THROW(parseCanonicalMoney("9223372036854775808"), std::out_of_range);
    EXPECT_THROW(parseCanonicalId<ActorTag>("18446744073709551615"), std::out_of_range);
    EXPECT_THROW(parseCanonicalSigned("-9223372036854775809"), std::out_of_range);
    EXPECT_THROW(parseCanonicalSigned("9223372036854775808"), std::out_of_range);
    EXPECT_THROW(detail::parseMagnitude("9", 0, 0), std::out_of_range);
}
