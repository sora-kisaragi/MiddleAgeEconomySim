#include <gtest/gtest.h>
#include <limits>
#include "market/market.h"

TEST(MarketAmountTest, Buy_UnrepresentableCost_LeavesStockAndPriceUnchanged) {
    Market market;
    Market control;
    market.sell("grain", 2, std::numeric_limits<int>::max());
    control.sell("grain", 2, std::numeric_limits<int>::max());
    EXPECT_THROW(market.buy("grain", 2), std::overflow_error);
    EXPECT_EQ(market.getStock("grain"), 2);
    EXPECT_EQ(market.getPrice("grain"), std::numeric_limits<int>::max());
    market.clearDaily();
    control.clearDaily();
    EXPECT_EQ(market.getPrice("grain"), control.getPrice("grain"));
}

TEST(MarketAmountTest, Transact_Uses64BitCost) {
    Market market;
    Person buyer;
    Business seller;
    seller.product = "grain";
    const int64_t cost = static_cast<int64_t>(std::numeric_limits<int>::max()) * 2;
    buyer.money = cost;
    seller.stock = 2;
    seller.price = std::numeric_limits<int>::max();
    ASSERT_TRUE(market.transact(&buyer, &seller, "grain", 2));
    EXPECT_EQ(buyer.money, 0);
    EXPECT_EQ(seller.money, cost);
    EXPECT_EQ(seller.stock, 0);
    EXPECT_EQ(market.getStock("grain"), 0);
}

TEST(MarketAmountTest, Transact_RecipientOverflow_DoesNotChangeBalancesOrMarket) {
    Market market;
    Person buyer;
    Business seller;
    seller.product = "grain";
    buyer.money = 100;
    seller.money = std::numeric_limits<int64_t>::max();
    seller.stock = 1;
    seller.price = 100;
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 1));
    EXPECT_EQ(buyer.money, 100);
    EXPECT_EQ(seller.money, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(seller.stock, 1);
    EXPECT_EQ(market.getPrice("grain"), 0);
    EXPECT_EQ(market.getStock("grain"), 0);
}

TEST(MarketAmountTest, Transact_UnrepresentableSellerPrice_IsRejected) {
    Market market;
    Person buyer;
    Business seller;
    seller.product = "grain";
    buyer.money = std::numeric_limits<int64_t>::max();
    seller.stock = 1;
    seller.price = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    EXPECT_FALSE(market.transact(&buyer, &seller, "grain", 1));
    EXPECT_EQ(market.getPrice("grain"), 0);
    EXPECT_EQ(seller.stock, 1);
}

TEST(MarketAmountTest, Sell_InvalidQuantityOrPrice_IsRejectedWithoutMutation) {
    Market market;
    market.sell("grain", 10, 100);
    for (int quantity : {0, -1}) {
        EXPECT_FALSE(market.sell("grain", quantity, 100));
        EXPECT_EQ(market.getStock("grain"), 10);
    }
    for (int price : {0, -1}) {
        EXPECT_FALSE(market.sell("new_product", 10, price));
        EXPECT_EQ(market.getStock("new_product"), 0);
        EXPECT_EQ(market.getPrice("new_product"), 0);
    }
}

TEST(MarketAmountTest, Sell_StockOverflow_IsRejectedWithoutMutation) {
    Market market;
    market.sell("grain", std::numeric_limits<int>::max(), 100);
    EXPECT_FALSE(market.sell("grain", 1, 100));
    EXPECT_EQ(market.getStock("grain"), std::numeric_limits<int>::max());
}

TEST(MarketAmountTest, RegisterProduct_InvalidPrice_PreservesExistingProduct) {
    Market market;
    market.sell("grain", 10, 100);
    for (int price : {0, -1}) {
        EXPECT_THROW(market.registerProduct("grain", price), std::invalid_argument);
        EXPECT_EQ(market.getStock("grain"), 10);
        EXPECT_EQ(market.getPrice("grain"), 100);
    }
}

TEST(MarketAmountTest, MultipleSellerOffers_StockOverflow_IsRejected) {
    Market market;
    Person buyer;
    buyer.money = std::numeric_limits<int64_t>::max();
    Business first;
    first.product = "grain";
    Business second;
    second.product = "grain";
    first.stock = std::numeric_limits<int>::max() - 1;
    second.stock = std::numeric_limits<int>::max();
    first.price = second.price = 1;
    EXPECT_FALSE(market.transact(&buyer, &first, "grain", std::numeric_limits<int>::max()));
    EXPECT_FALSE(market.transact(&buyer, &second, "grain", 1));
    EXPECT_EQ(market.getStock("grain"), std::numeric_limits<int>::max() - 1);
    EXPECT_EQ(buyer.money, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(second.stock, std::numeric_limits<int>::max());
}
