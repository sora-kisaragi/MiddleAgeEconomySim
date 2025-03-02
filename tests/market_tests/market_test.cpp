#include <gtest/gtest.h>
#include "market/market.h"

TEST(MarketTest, DefaultConstructor) {
    Market m;
    EXPECT_TRUE(m.stock.empty());
    EXPECT_TRUE(m.price.empty());
}

TEST(MarketTest, AddProduct) {
    Market m;
    m.stock["Wheat"] = 100;
    m.price["Wheat"] = 5;
    
    EXPECT_EQ(1, m.stock.size());
    EXPECT_EQ(1, m.price.size());
    EXPECT_EQ(100, m.stock["Wheat"]);
    EXPECT_EQ(5, m.price["Wheat"]);
}

TEST(MarketTest, MultipleProducts) {
    Market m;
    m.stock["Wheat"] = 100;
    m.price["Wheat"] = 5;
    m.stock["Bread"] = 50;
    m.price["Bread"] = 10;
    
    EXPECT_EQ(2, m.stock.size());
    EXPECT_EQ(2, m.price.size());
    EXPECT_EQ(100, m.stock["Wheat"]);
    EXPECT_EQ(5, m.price["Wheat"]);
    EXPECT_EQ(50, m.stock["Bread"]);
    EXPECT_EQ(10, m.price["Bread"]);
}
