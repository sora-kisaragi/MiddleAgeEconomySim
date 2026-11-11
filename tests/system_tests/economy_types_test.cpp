#include "core/economy_types.h"

#include <gtest/gtest.h>
#include <limits>
#include <type_traits>

using namespace economy;

static_assert(!std::is_convertible<ActorId, AccountId>::value, "ID kinds must not be interchangeable");
static_assert(!std::is_convertible<Money, Quantity>::value, "units must not be interchangeable");
static_assert(!std::is_convertible<std::int64_t, Money>::value, "amount conversion must be explicit");

TEST(EconomyTypesTest, TypedIdsRejectZeroAndPreserveConfigurationOrderIndependence) {
    EXPECT_THROW(ActorId(0), std::invalid_argument);
    EXPECT_THROW(ActorId(std::uint64_t{1} << 63), std::invalid_argument);
    const auto first = assignStableIds<ActorTag>({"merchant", "farmer", "baker"});
    const auto second = assignStableIds<ActorTag>({"baker", "merchant", "farmer"});
    EXPECT_EQ(first.at("baker"), second.at("baker"));
    EXPECT_EQ(first.at("baker").value(), 1U);
    EXPECT_EQ(first.at("merchant").value(), 3U);
    EXPECT_THROW((assignStableIds<ActorTag>({"baker", "baker"})), std::invalid_argument);
    EXPECT_THROW((assignStableIds<ActorTag>({""})), std::invalid_argument);
}

TEST(EconomyTypesTest, AmountsRejectNegativeAndCheckIntermediateArithmetic) {
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    EXPECT_THROW(Money(-1), std::invalid_argument);
    EXPECT_THROW(Quantity(-1), std::invalid_argument);
    EXPECT_EQ(Money(8).add(Money(3)).value(), 11);
    EXPECT_EQ(Quantity(8).subtract(Quantity(3)).value(), 5);
    EXPECT_EQ(Money(5).multiply(4).value(), 20);
    EXPECT_THROW(Money(max).add(Money(1)), std::overflow_error);
    EXPECT_THROW(Money(0).subtract(Money(1)), std::underflow_error);
    EXPECT_THROW(Quantity(max).multiply(2), std::overflow_error);
    EXPECT_THROW(Money(1).multiply(-1), std::invalid_argument);
}

TEST(EconomyTypesTest, RatesUseCheckedIntegerMultiplicationAndFloor) {
    EXPECT_EQ(Rate(3, 4).applyFloor(Money(10)).value(), 7);
    EXPECT_THROW(Rate(-1, 1), std::invalid_argument);
    EXPECT_THROW(Rate(1, 0), std::invalid_argument);
    EXPECT_THROW(Rate(2, 3).applyFloor(Money(std::numeric_limits<std::int64_t>::max())),
                 std::overflow_error);
}

TEST(EconomyTypesTest, GameTimeUsesOnlySimulatedDaysAndPhases) {
    EXPECT_TRUE(GameTime(0, 0, 0) < GameTime(1, 1, 0));
    EXPECT_TRUE(GameTime(1, 1, 2) < GameTime(1, 2, 0));
    EXPECT_EQ(GameTime(1, 12, 1), GameTime(1, 12, 1));
    EXPECT_THROW(GameTime(0, 1, 0), std::invalid_argument);
    EXPECT_THROW(GameTime(1, 0, 0), std::invalid_argument);
    EXPECT_THROW(GameTime(1, 13, 0), std::invalid_argument);
    EXPECT_THROW(GameTime(1, 256, 0), std::invalid_argument);
}
