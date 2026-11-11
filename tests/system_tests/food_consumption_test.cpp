#include <gtest/gtest.h>
#include "system/simulation.h"

class FoodConsumptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        people.resize(1);
        businesses.resize(1);
        businesses[0].product = "小麦";
        businesses[0].price = 10;
    }
    void runDay() {
        ASSERT_TRUE(simulateDay(people, businesses, market, government, lender, routes));
    }
    std::vector<Person> people;
    std::vector<Business> businesses;
    Market market;
    Government government;
    LoanProvider lender;
    std::vector<TradeRoute> routes;
};

TEST_F(FoodConsumptionTest, NoFood_DoesNotConsumeToolsOrRaiseSatisfaction) {
    people[0].inventory = {"道具"};
    runDay();
    EXPECT_EQ(people[0].inventory, (std::vector<std::string>{"道具"}));
    EXPECT_EQ(people[0].satisfaction, 45);
}
TEST_F(FoodConsumptionTest, StoredFood_ConsumesOnlyOneAndKeepsOtherItems) {
    people[0].inventory = {"道具", "小麦", "パン", "小麦"};
    runDay();
    EXPECT_EQ(people[0].inventory, (std::vector<std::string>{"道具", "パン", "小麦"}));
    EXPECT_EQ(people[0].satisfaction, 60);
}
TEST_F(FoodConsumptionTest, PurchasedFood_IsConsumedWithoutRemovingTools) {
    people[0].inventory = {"道具"};
    people[0].money = 100;
    businesses[0].stock = 1;
    runDay();
    EXPECT_EQ(people[0].inventory, (std::vector<std::string>{"道具"}));
    EXPECT_EQ(people[0].satisfaction, 60);
    EXPECT_EQ(people[0].money, 90);
    EXPECT_EQ(businesses[0].money, 10);
    EXPECT_EQ(businesses[0].stock, 0);
}
TEST_F(FoodConsumptionTest, FoodReserve_LastTwoDaysThenHunger) {
    people[0].inventory = {"道具", "小麦", "小麦"};
    runDay();
    EXPECT_EQ(people[0].satisfaction, 60);
    runDay();
    EXPECT_EQ(people[0].satisfaction, 70);
    runDay();
    EXPECT_EQ(people[0].satisfaction, 65);
    EXPECT_EQ(people[0].inventory, (std::vector<std::string>{"道具"}));
}
