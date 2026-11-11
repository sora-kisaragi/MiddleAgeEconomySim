#include <gtest/gtest.h>
#include <limits>
#include "market/business.h"

TEST(BusinessFinanceTest, Payroll_WithoutWorkers_ThrowsBeforeDivision) {
    Business business;
    business.money = 100;
    for (int workers : {0, -1}) {
        business.workers = workers;
        EXPECT_THROW(business.payWorkers(10), std::runtime_error);
        EXPECT_EQ(business.money, 100);
    }
}

TEST(BusinessFinanceTest, Sale_RecipientOverflow_PreservesStockAndMoney) {
    Business business;
    business.money = std::numeric_limits<int64_t>::max();
    business.stock = 10;
    business.price = 1;
    EXPECT_THROW(business.sell(1), std::overflow_error);
    EXPECT_EQ(business.stock, 10);
    EXPECT_EQ(business.money, std::numeric_limits<int64_t>::max());
}

TEST(BusinessFinanceTest, Sale_MultiplicationOverflow_PreservesStockAndMoney) {
    Business business;
    business.stock = 10;
    business.price = std::numeric_limits<int64_t>::max();
    EXPECT_THROW(business.sell(2), std::overflow_error);
    EXPECT_EQ(business.stock, 10);
    EXPECT_EQ(business.money, 0);
}

TEST(BusinessFinanceTest, Sale_NegativePublicPrice_IsRejected) {
    Business business;
    business.stock = 10;
    business.price = -1;
    EXPECT_THROW(business.sell(1), std::invalid_argument);
    EXPECT_EQ(business.stock, 10);
    EXPECT_EQ(business.money, 0);
}

TEST(BusinessFinanceTest, Sale_MaximumRevenue_Succeeds) {
    Business business;
    business.stock = 1;
    business.price = std::numeric_limits<int64_t>::max();
    EXPECT_NO_THROW(business.sell(1));
    EXPECT_EQ(business.stock, 0);
    EXPECT_EQ(business.money, std::numeric_limits<int64_t>::max());
}

TEST(BusinessFinanceTest, Sale_ZeroPrice_RemainsSupported) {
    Business business;
    business.stock = 1;
    business.price = 0;
    EXPECT_NO_THROW(business.sell(1));
    EXPECT_EQ(business.stock, 0);
    EXPECT_EQ(business.money, 0);
}
