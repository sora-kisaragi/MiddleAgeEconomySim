#include <gtest/gtest.h>
#include <limits>
#include "market/market.h"

TEST(DailyMarketTest, SplitPurchases_EqualVolumes_EqualClosingPrices) {
    Market single;
    Market split;
    single.sell("grain", 100, 100);
    split.sell("grain", 40, 100);
    split.sell("grain", 60, 100);
    EXPECT_EQ(single.buy("grain", 100), 10000);
    EXPECT_EQ(split.buy("grain", 50), 5000);
    EXPECT_EQ(split.buy("grain", 50), 5000);
    single.clearDaily();
    split.clearDaily();
    EXPECT_EQ(single.getPrice("grain"), 100);
    EXPECT_EQ(split.getPrice("grain"), 100);
    EXPECT_EQ(single.getStock("grain"), 0);
    EXPECT_EQ(split.getStock("grain"), 0);
}

TEST(DailyMarketTest, Oversupply_UpdatesOnlyAtClose) {
    Market market;
    market.sell("grain", 100, 100);
    market.buy("grain", 50);
    EXPECT_EQ(market.getPrice("grain"), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 95);
}

TEST(DailyMarketTest, UnfilledDemand_IncreasesClosingPrice) {
    Market market;
    market.sell("grain", 100, 100);
    market.buy("grain", 100);
    EXPECT_THROW(market.buy("grain", 100), std::invalid_argument);
    EXPECT_EQ(market.getPrice("grain"), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, CarryoverStock_IsIncludedInNextDaySupply) {
    Market market;
    market.sell("grain", 100, 100);
    market.buy("grain", 50);
    market.clearDaily();
    ASSERT_EQ(market.getPrice("grain"), 95);
    market.sell("grain", 50, 100);
    EXPECT_EQ(market.buy("grain", 100), 9500);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 95);
}

TEST(DailyMarketTest, ExplicitUpdate_RepeatedCalls_DoNotCompound) {
    Market market;
    market.sell("grain", 100, 100);
    market.buy("grain", 50);
    market.updatePrice("grain");
    EXPECT_EQ(market.getPrice("grain"), 95);
    market.updatePrice("grain");
    EXPECT_EQ(market.getPrice("grain"), 95);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 95);
}

TEST(DailyMarketTest, Products_HaveIndependentDailyTotals) {
    Market market;
    market.sell("grain", 100, 100);
    market.sell("tools", 100, 200);
    market.buy("grain", 50);
    market.buy("tools", 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 95);
    EXPECT_EQ(market.getPrice("tools"), 200);
}

TEST(DailyMarketTest, EmptyMarket_NoDemand_KeepsPrice) {
    Market market;
    market.registerProduct("grain", 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 100);
}

TEST(DailyMarketTest, ZeroSupply_WithDemand_UsesSafeDenominator) {
    Market market;
    market.registerProduct("grain", 100);
    EXPECT_THROW(market.buy("grain", 1), std::invalid_argument);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, SuccessiveDays_UsePreviousClosingPrice) {
    Market market;
    market.sell("grain", 100, 100);
    for (int expected_price : {110, 121}) {
        EXPECT_THROW(market.buy("grain", 200), std::invalid_argument);
        market.clearDaily();
        EXPECT_EQ(market.getPrice("grain"), expected_price);
    }
}

TEST(DailyMarketTest, ExtremeDemand_ClosingPriceIsBounded) {
    Market market;
    market.registerProduct("grain", 100);
    EXPECT_THROW(market.buy("grain", std::numeric_limits<int>::max()), std::invalid_argument);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), std::numeric_limits<int>::max());
}

TEST(DailyMarketTest, FailedFunding_DoesNotCreateDemand) {
    Market market;
    Person buyer;
    Business seller;
    seller.stock = 100;
    seller.price = 100;
    market.sell("grain", 100, 100);
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 100));
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 90);
}

TEST(DailyMarketTest, Transactions_UseOpeningPriceAndAccumulateDemand) {
    Market market;
    Person buyer;
    Business seller;
    buyer.money = 10000;
    seller.stock = 100;
    seller.price = 100;
    market.sell("grain", 100, 100);
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 50));
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 50));
    EXPECT_EQ(buyer.money, 0);
    EXPECT_EQ(seller.money, 10000);
    EXPECT_EQ(seller.stock, 0);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 100);
}

TEST(DailyMarketTest, Transaction_StockShortage_RecordsAffordableDemand) {
    Market market;
    Person buyer;
    Business seller;
    buyer.money = 20000;
    seller.stock = 100;
    seller.price = 100;
    market.sell("grain", 100, 100);
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    EXPECT_EQ(buyer.money, 20000);
    EXPECT_EQ(seller.stock, 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, Oversupply_PriceHasPositiveLowerBound) {
    Market market;
    market.setPriceVolatility(2.0f);
    market.sell("grain", 100, 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 1);
}

TEST(DailyMarketTest, UnlistedSeller_SplitRequests_HaveEqualSupplyAndPrice) {
    Market single;
    Market split;
    Person single_buyer;
    Person split_buyer;
    single_buyer.money = split_buyer.money = 40000;
    Business single_seller;
    Business split_seller;
    single_seller.stock = split_seller.stock = 100;
    single_seller.price = split_seller.price = 100;
    EXPECT_FALSE(single.transact(&single_buyer, &single_seller, "grain", 200));
    ASSERT_TRUE(split.transact(&split_buyer, &split_seller, "grain", 100));
    EXPECT_FALSE(split.transact(&split_buyer, &split_seller, "grain", 100));
    single.clearDaily();
    split.clearDaily();
    EXPECT_EQ(single.getPrice("grain"), 110);
    EXPECT_EQ(split.getPrice("grain"), 110);
}

TEST(DailyMarketTest, UnlistedSeller_RepeatedShortages_DoNotDuplicateSupply) {
    Market market;
    Person buyer;
    buyer.money = 40000;
    Business seller;
    seller.stock = 100;
    seller.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    EXPECT_EQ(market.getStock("grain"), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 130);
}

TEST(DailyMarketTest, MultipleSellers_OfferDistinctSupplyAcrossDays) {
    Market market;
    Person buyer;
    buyer.money = 40000;
    Business first;
    Business second;
    first.stock = second.stock = 100;
    first.price = second.price = 100;
    for (int expected_price : {110, 121}) {
        EXPECT_FALSE(market.transact(&buyer, &first, "grain", 200));
        EXPECT_FALSE(market.transact(&buyer, &second, "grain", 200));
        EXPECT_EQ(market.getStock("grain"), 200);
        market.clearDaily();
        EXPECT_EQ(market.getPrice("grain"), expected_price);
    }
}

TEST(DailyMarketTest, PrelistedSupply_IsAssignedWithoutDoubleCounting) {
    Market market;
    market.sell("grain", 200, 100);
    Person buyer;
    buyer.money = 40000;
    Business first;
    Business second;
    first.stock = second.stock = 100;
    first.price = second.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &first, "grain", 200));
    EXPECT_FALSE(market.transact(&buyer, &second, "grain", 200));
    EXPECT_EQ(market.getStock("grain"), 200);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, Seller_SuccessThenShortage_DoesNotRelistSoldGoods) {
    Market market;
    Person buyer;
    buyer.money = 40000;
    Business seller;
    seller.stock = 100;
    seller.price = 100;
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 50));
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 100));
    EXPECT_EQ(market.getStock("grain"), 50);
    EXPECT_EQ(seller.stock, 50);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 105);
}

TEST(DailyMarketTest, AnonymousPurchase_ExhaustsMarket_DoesNotRegenerateListedGoods) {
    Market market;
    Person buyer;
    buyer.money = 40000;
    Business seller;
    seller.stock = 100;
    seller.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    market.buy("grain", 100);
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 100));
    EXPECT_EQ(market.getStock("grain"), 0);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 130);
}
