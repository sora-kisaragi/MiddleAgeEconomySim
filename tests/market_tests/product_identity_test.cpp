#include <gtest/gtest.h>
#include "market/market.h"

TEST(ProductIdentityTest, WrongSellerProduct_PreservesCashStockAndDailyPrice) {
    Market market;
    Market control;
    for (auto* current : {&market, &control}) {
        current->sell("grain", 10, 100);
        current->sell("bread", 10, 50);
    }
    Person buyer;
    buyer.money = 1000;
    Business bakery;
    bakery.product = "bread";
    bakery.price = 50;
    bakery.stock = 10;
    EXPECT_FALSE(market.transact(&buyer, &bakery, "grain", 1));
    EXPECT_EQ(buyer.money, 1000);
    EXPECT_EQ(bakery.money, 0);
    EXPECT_EQ(bakery.stock, 10);
    EXPECT_EQ(market.getStock("grain"), 10);
    EXPECT_EQ(market.getStock("bread"), 10);
    market.clearDaily();
    control.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), control.getPrice("grain"));
    EXPECT_EQ(market.getPrice("bread"), control.getPrice("bread"));
}
TEST(ProductIdentityTest, MismatchedUnknownProduct_DoesNotRegisterOrListIt) {
    for (const std::string seller_product : {"bread", ""}) {
        Market market;
        Person buyer;
        buyer.money = 1000;
        Business seller;
        seller.product = seller_product;
        seller.price = 50;
        seller.stock = 10;
        EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 1));
        EXPECT_EQ(market.getPrice("grain"), 0);
        EXPECT_EQ(market.getStock("grain"), 0);
        EXPECT_EQ(seller.stock, 10);
        EXPECT_EQ(buyer.money, 1000);
        EXPECT_EQ(seller.money, 0);
    }
}
TEST(ProductIdentityTest, EmptyProductName_IsRejectedAtMarketEntries) {
    Market market;
    EXPECT_THROW(market.registerProduct("", 100), std::invalid_argument);
    EXPECT_FALSE(market.sell("", 10, 100));
    Person buyer;
    buyer.money = 1000;
    Business seller;
    seller.price = 100;
    seller.stock = 10;
    EXPECT_FALSE(market.transact(&buyer, &seller, "", 1));
    EXPECT_EQ(market.getPrice(""), 0);
    EXPECT_EQ(market.getStock(""), 0);
    EXPECT_EQ(seller.stock, 10);
    EXPECT_EQ(buyer.money, 1000);
}
TEST(ProductIdentityTest, MatchingUnicodeProduct_TradesNormally) {
    Market market;
    Person buyer;
    buyer.money = 100;
    Business seller;
    seller.product = "小麦";
    seller.stock = 10;
    seller.price = 5;
    EXPECT_TRUE(market.transact(&buyer, &seller, "小麦", 2));
    EXPECT_EQ(buyer.money, 90);
    EXPECT_EQ(seller.money, 10);
    EXPECT_EQ(seller.stock, 8);
    EXPECT_EQ(market.getStock("小麦"), 8);
}
