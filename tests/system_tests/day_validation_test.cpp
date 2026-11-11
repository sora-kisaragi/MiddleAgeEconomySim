#include <gtest/gtest.h>
#include <limits>
#include "system/simulation.h"

class DayValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        people.resize(1);
        people[0].money = 100;
        people[0].daily_income = 5;
        businesses.resize(2);
        for (auto& business : businesses) {
            business.product = "小麦";
            business.stock = 3;
            business.daily_production = 2;
            business.price = 10;
            business.money = 100;
        }
        market.registerProduct("小麦", 12);
        market.sell("小麦", 6, 12);
    }
    void expectRejectedWithoutMutation() {
        const auto original = businesses;
        const int64_t person_cash = people[0].money;
        EXPECT_FALSE(simulateDay(people, businesses, market, government, lender, routes));
        for (size_t i = 0; i < businesses.size(); ++i) {
            EXPECT_EQ(businesses[i].stock, original[i].stock);
            EXPECT_EQ(businesses[i].money, original[i].money);
        }
        EXPECT_EQ(people[0].money, person_cash);
        EXPECT_EQ(market.getStock("小麦"), 6);
        EXPECT_EQ(market.getPrice("小麦"), 12);
    }
    std::vector<Person> people;
    std::vector<Business> businesses;
    Market market;
    Government government;
    LoanProvider lender;
    std::vector<TradeRoute> routes;
};

TEST_F(DayValidationTest, InvalidLateBusinessPrice_IsRejectedBeforeEarlierProduction) {
    businesses[1].price = 0;
    expectRejectedWithoutMutation();
}
TEST_F(DayValidationTest, InvalidStockAndProduction_AreRejectedBeforeMutation) {
    for (int invalid : {-1, std::numeric_limits<int>::min()}) {
        businesses[1].stock = invalid;
        expectRejectedWithoutMutation();
        businesses[1].stock = 3;
        businesses[1].daily_production = invalid;
        expectRejectedWithoutMutation();
        businesses[1].daily_production = 2;
    }
}
TEST_F(DayValidationTest, ProductStockSumOverflow_IsRejectedBeforeMutation) {
    businesses[1].stock = std::numeric_limits<int>::max() - 3;
    expectRejectedWithoutMutation();
}
TEST_F(DayValidationTest, IndividualProductionOverflow_IsRejectedBeforeMutation) {
    businesses[1].stock = std::numeric_limits<int>::max();
    expectRejectedWithoutMutation();
}
TEST_F(DayValidationTest, EmptyProduct_IsRejectedBeforeMutation) {
    businesses[1].product.clear();
    expectRejectedWithoutMutation();
}

TEST_F(DayValidationTest, ValidDay_ReturnsSuccess) {
    EXPECT_TRUE(simulateDay(people, businesses, market, government, lender, routes));
}
TEST_F(DayValidationTest, EmptyAgents_ReturnFailure) {
    people.clear();
    EXPECT_FALSE(simulateDay(people, businesses, market, government, lender, routes));
    people.resize(1);
    businesses.clear();
    EXPECT_FALSE(simulateDay(people, businesses, market, government, lender, routes));
}
TEST_F(DayValidationTest, OutOfRangePrice_IsRejectedBeforeMutation) {
    businesses[1].price = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    expectRejectedWithoutMutation();
}
TEST_F(DayValidationTest, MaximumAggregateStock_IsAccepted) {
    businesses[0].stock = std::numeric_limits<int>::max() - 7;
    businesses[0].daily_production = 2;
    EXPECT_TRUE(simulateDay(people, businesses, market, government, lender, routes));
    EXPECT_EQ(market.getStock("小麦"), std::numeric_limits<int>::max() - 1);
}

TEST_F(DayValidationTest, InvalidNpcSatisfaction_IsRejectedBeforeMutation) {
    people.push_back(people[0]);
    for (int invalid : {-1, 101, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        people[1].satisfaction = invalid;
        expectRejectedWithoutMutation();
        EXPECT_EQ(people[1].satisfaction, invalid);
    }
}

TEST_F(DayValidationTest, ValidNpcSatisfactionEndpoints_RemainWithinRange) {
    for (int endpoint : {0, 100}) {
        people[0].satisfaction = endpoint;
        people[0].money = 0;
        EXPECT_TRUE(simulateDay(people, businesses, market, government, lender, routes));
        EXPECT_EQ(people[0].satisfaction, std::max(0, endpoint - 5));
    }
}

TEST_F(DayValidationTest, ValidNpcSatisfactionEndpoints_WithFoodRemainWithinRange) {
    people[0].daily_income = 0;
    for (int endpoint : {0, 100}) {
        people[0].satisfaction = endpoint;
        people[0].money = 0;
        people[0].inventory = {"小麦"};
        EXPECT_TRUE(simulateDay(people, businesses, market, government, lender, routes));
        EXPECT_EQ(people[0].satisfaction, std::min(100, endpoint + 10));
        EXPECT_TRUE(people[0].inventory.empty());
    }
}
