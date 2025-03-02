#include <gtest/gtest.h>
#include "market/business.h"

TEST(BusinessTest, DefaultConstructor) {
    Business b;
    EXPECT_EQ(0, b.id);
    EXPECT_EQ("", b.product);
    EXPECT_EQ(0, b.stock);
    EXPECT_EQ(0, b.price);
    EXPECT_EQ(0, b.workers);
    EXPECT_EQ(0, b.daily_production);
}

TEST(BusinessTest, CustomBusiness) {
    Business b;
    b.id = 1;
    b.product = "Wheat";
    b.stock = 100;
    b.price = 5;
    b.workers = 10;
    b.daily_production = 20;
    
    EXPECT_EQ(1, b.id);
    EXPECT_EQ("Wheat", b.product);
    EXPECT_EQ(100, b.stock);
    EXPECT_EQ(5, b.price);
    EXPECT_EQ(10, b.workers);
    EXPECT_EQ(20, b.daily_production);
}
