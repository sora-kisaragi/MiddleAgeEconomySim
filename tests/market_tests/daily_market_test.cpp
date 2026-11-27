#include "../owned_market_fixture.h"
#include <gtest/gtest.h>
#include <limits>
#include "market/market.h"

TEST(DailyMarketTest, SplitPurchases_EqualVolumes_EqualClosingPrices) {
    OwnedMarket single;
    OwnedMarket split;
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
    OwnedMarket market;
    market.sell("grain", 100, 100);
    market.buy("grain", 50);
    EXPECT_EQ(market.getPrice("grain"), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 95);
}

TEST(DailyMarketTest, UnfilledDemand_IncreasesClosingPrice) {
    OwnedMarket market;
    market.sell("grain", 100, 100);
    market.buy("grain", 100);
    EXPECT_THROW(market.buy("grain", 100), std::invalid_argument);
    EXPECT_EQ(market.getPrice("grain"), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, CarryoverStock_IsIncludedInNextDaySupply) {
    OwnedMarket market;
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
    OwnedMarket market;
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
    OwnedMarket market;
    market.sell("grain", 100, 100);
    market.sell("tools", 100, 200);
    market.buy("grain", 50);
    market.buy("tools", 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 95);
    EXPECT_EQ(market.getPrice("tools"), 200);
}

TEST(DailyMarketTest, EmptyMarket_NoDemand_KeepsPrice) {
    OwnedMarket market;
    market.registerProduct("grain", 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 100);
}

TEST(DailyMarketTest, ZeroSupply_WithDemand_UsesSafeDenominator) {
    OwnedMarket market;
    market.registerProduct("grain", 100);
    EXPECT_THROW(market.buy("grain", 1), std::invalid_argument);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, SuccessiveDays_UsePreviousClosingPrice) {
    OwnedMarket market;
    market.sell("grain", 100, 100);
    for (int expected_price : {110, 121}) {
        EXPECT_THROW(market.buy("grain", 200), std::invalid_argument);
        market.clearDaily();
        EXPECT_EQ(market.getPrice("grain"), expected_price);
    }
}

TEST(DailyMarketTest, ExtremeDemand_ClosingPriceIsBounded) {
    OwnedMarket market;
    market.registerProduct("grain", 100);
    EXPECT_THROW(market.buy("grain", std::numeric_limits<int>::max()), std::invalid_argument);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), std::numeric_limits<int>::max());
}

TEST(DailyMarketTest, FailedFunding_DoesNotCreateDemand) {
    OwnedMarket market;
    Person buyer;
    Business seller;
    seller.product = "grain";
    seller.stock = 100;
    seller.price = 100;
    market.sell("grain", 100, 100);
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 100));
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 90);
}

TEST(DailyMarketTest, Transactions_UseOpeningPriceAndAccumulateDemand) {
    OwnedMarket market;
    Person buyer;
    Business seller;
    seller.product = "grain";
    buyer.money = 10000;
    seller.stock = 100;
    seller.price = 100;
    market.sell("grain", 100, 100);
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 50));
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 50));
    EXPECT_EQ(buyer.money, 0);
    EXPECT_EQ(seller.money, 10000);
    EXPECT_EQ(seller.goodsStock(), 0);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 100);
}

TEST(DailyMarketTest, Transaction_StockShortage_RecordsAffordableDemand) {
    OwnedMarket market;
    Person buyer;
    Business seller;
    seller.product = "grain";
    buyer.money = 20000;
    seller.stock = 100;
    seller.price = 100;
    market.sell("grain", 100, 100);
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    EXPECT_EQ(buyer.money, 20000);
    EXPECT_EQ(seller.goodsStock(), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, Oversupply_PriceHasPositiveLowerBound) {
    OwnedMarket market;
    market.setPriceVolatility(2.0f);
    market.sell("grain", 100, 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 1);
}

TEST(DailyMarketTest, UnlistedSeller_SplitRequests_HaveEqualSupplyAndPrice) {
    OwnedMarket single;
    OwnedMarket split;
    Person single_buyer;
    Person split_buyer;
    single_buyer.money = split_buyer.money = 40000;
    Business single_seller;
    single_seller.product = "grain";
    Business split_seller;
    split_seller.product = "grain";
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
    OwnedMarket market;
    Person buyer;
    buyer.money = 40000;
    Business seller;
    seller.product = "grain";
    seller.stock = 100;
    seller.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    EXPECT_EQ(market.getStock("grain"), 100);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 130);
}

TEST(DailyMarketTest, MultipleSellers_OfferDistinctSupplyAcrossDays) {
    OwnedMarket market;
    Person buyer;
    buyer.money = 40000;
    Business first;
    first.product = "grain";
    Business second;
    second.product = "grain";
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
    OwnedMarket market;
    market.sell("grain", 200, 100);
    Person buyer;
    buyer.money = 40000;
    Business first;
    first.product = "grain";
    Business second;
    second.product = "grain";
    first.stock = second.stock = 100;
    first.price = second.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &first, "grain", 200));
    EXPECT_FALSE(market.transact(&buyer, &second, "grain", 200));
    EXPECT_EQ(market.getStock("grain"), 200);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 110);
}

TEST(DailyMarketTest, Seller_SuccessThenShortage_DoesNotRelistSoldGoods) {
    OwnedMarket market;
    Person buyer;
    buyer.money = 40000;
    Business seller;
    seller.product = "grain";
    seller.stock = 100;
    seller.price = 100;
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 50));
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 100));
    EXPECT_EQ(market.getStock("grain"), 50);
    EXPECT_EQ(seller.goodsStock(), 50);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 105);
}

TEST(DailyMarketTest, OwnerBackedPurchase_ExhaustsMarket_DoesNotRegenerateListedGoods) {
    OwnedMarket market;
    Person buyer;
    buyer.money = 40000;
    Business seller;
    seller.product = "grain";
    seller.stock = 100;
    seller.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 200));
    market.buy("grain", 100);
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 100));
    EXPECT_EQ(market.getStock("grain"), 0);
    market.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), 130);
}

TEST(DailyMarketTest, RegisterProduct_RepricingPreservesBothProductsOwnerInventory) {
    OwnedMarket market;
    Person buyer;
    buyer.money = 40000;
    Business farmer;
    farmer.product = "grain";
    Business artisan;
    artisan.product = "tools";
    farmer.stock = artisan.stock = 100;
    farmer.price = artisan.price = 100;
    ASSERT_TRUE(market.transact(&buyer, &farmer, "grain", 50));
    ASSERT_TRUE(market.transact(&buyer, &artisan, "tools", 50));
    market.registerProduct("grain", 100);
    ASSERT_TRUE(market.transact(&buyer, &farmer, "grain", 25));
    ASSERT_TRUE(market.transact(&buyer, &artisan, "tools", 25));
    EXPECT_EQ(market.getStock("grain"), 25);
    EXPECT_EQ(farmer.goodsStock(), 25);
    EXPECT_EQ(market.getStock("tools"), 25);
    EXPECT_EQ(artisan.goodsStock(), 25);
}
