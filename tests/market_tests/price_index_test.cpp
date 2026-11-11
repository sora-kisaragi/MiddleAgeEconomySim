#include <gtest/gtest.h>
#include <limits>
#include "market/price_index.h"

TEST(PriceIndexTest, ReferenceBasket_StartsAtOneHundred) {
    Market market;
    market.registerProduct("grain", 100);
    PriceIndex index(market, {{"grain", 1}});
    const auto observation = index.observe(market);
    EXPECT_DOUBLE_EQ(observation.value, 100.0);
    EXPECT_DOUBLE_EQ(observation.change_percent, 0.0);
}

TEST(PriceIndexTest, FixedQuantities_WeightTheBasketValue) {
    Market market;
    market.registerProduct("grain", 100);
    market.registerProduct("tools", 200);
    PriceIndex index(market, {{"grain", 2}, {"tools", 1}});
    market.registerProduct("grain", 200);
    market.registerProduct("tools", 100);
    const auto observation = index.observe(market);
    EXPECT_DOUBLE_EQ(observation.value, 125.0);
    EXPECT_DOUBLE_EQ(observation.change_percent, 25.0);
}

TEST(PriceIndexTest, DailyChange_UsesPreviousObservation) {
    Market market;
    market.registerProduct("grain", 100);
    PriceIndex index(market, {{"grain", 1}});
    market.registerProduct("grain", 110);
    EXPECT_NEAR(index.observe(market).change_percent, 10.0, 1e-10);
    market.registerProduct("grain", 121);
    const auto observation = index.observe(market);
    EXPECT_NEAR(observation.value, 121.0, 1e-10);
    EXPECT_NEAR(observation.change_percent, 10.0, 1e-10);
}

TEST(PriceIndexTest, FallingPrices_ProduceNegativeDailyChange) {
    Market market;
    market.registerProduct("grain", 100);
    PriceIndex index(market, {{"grain", 1}});
    market.registerProduct("grain", 90);
    const auto observation = index.observe(market);
    EXPECT_DOUBLE_EQ(observation.value, 90.0);
    EXPECT_NEAR(observation.change_percent, -10.0, 1e-10);
}

TEST(PriceIndexTest, UnrelatedProducts_DoNotChangeBasketOrReference) {
    Market market;
    market.registerProduct("grain", 100);
    PriceIndex index(market, {{"grain", 1}});
    market.registerProduct("luxury", 100000);
    EXPECT_DOUBLE_EQ(index.observe(market).value, 100.0);
}

TEST(PriceIndexTest, EmptyOrNonPositiveQuantities_AreRejected) {
    Market market;
    market.registerProduct("grain", 100);
    EXPECT_THROW((PriceIndex(market, {})), std::invalid_argument);
    EXPECT_THROW((PriceIndex(market, {{"grain", 0}})), std::invalid_argument);
    EXPECT_THROW((PriceIndex(market, {{"grain", -1}})), std::invalid_argument);
}

TEST(PriceIndexTest, MissingReferenceProduct_IsRejected) {
    Market market;
    EXPECT_THROW((PriceIndex(market, {{"grain", 1}})), std::invalid_argument);
}

TEST(PriceIndexTest, FailedObservation_DoesNotAdvancePreviousIndex) {
    Market market;
    market.registerProduct("grain", 100);
    PriceIndex index(market, {{"grain", 1}});
    market.registerProduct("grain", 110);
    index.observe(market);
    Market incomplete;
    EXPECT_THROW(index.observe(incomplete), std::invalid_argument);
    market.registerProduct("grain", 121);
    EXPECT_NEAR(index.observe(market).change_percent, 10.0, 1e-10);
}

TEST(PriceIndexTest, LargePricesAndQuantities_DoNotOverflowIntegerArithmetic) {
    Market market;
    const int maximum = std::numeric_limits<int>::max();
    market.registerProduct("grain", maximum);
    market.registerProduct("tools", maximum);
    market.registerProduct("bread", maximum);
    PriceIndex index(market, {{"grain", maximum}, {"tools", maximum}, {"bread", maximum}});
    EXPECT_DOUBLE_EQ(index.observe(market).value, 100.0);
}
