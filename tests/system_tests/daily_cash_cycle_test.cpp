#include <gtest/gtest.h>
#include "system/simulation.h"

class DailyCashCycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        Person person;
        person.money = 100;
        person.daily_income = 50;
        people.push_back(person);
        Business farm;
        farm.product = "小麦";
        farm.daily_production = 10;
        farm.price = 5;
        farm.money = 1000;
        businesses.push_back(farm);
        government.money = 1000;
        lender.money = 5000;
    }

    int64_t totalMoney() const {
        int64_t total = government.money + lender.money;
        for (const auto& person : people) total += person.money;
        for (const auto& business : businesses) total += business.money;
        return total;
    }

    void runDay() { simulateDay(people, businesses, market, government, lender, routes); }

    std::vector<Person> people;
    std::vector<Business> businesses;
    Market market;
    Government government;
    LoanProvider lender;
    std::vector<TradeRoute> routes;
};

TEST_F(DailyCashCycleTest, EconomicActivity_ConservesTotalMoneyAcrossDays) {
    Person second = people[0];
    second.daily_income = 60;
    people.push_back(second);
    Business second_business = businesses[0];
    second_business.product = "パン";
    businesses.push_back(second_business);
    const int64_t initial = totalMoney();
    for (int day = 0; day < 5; ++day) {
        SCOPED_TRACE(day);
        runDay();
        EXPECT_EQ(totalMoney(), initial);
    }
}

TEST_F(DailyCashCycleTest, Purchase_CreditsSellerAndConsumesItsStock) {
    people[0].daily_income = 0;
    businesses[0].money = 0;
    government.money = lender.money = 0;
    runDay();
    EXPECT_EQ(people[0].money, 95);
    EXPECT_EQ(businesses[0].money, 5);
    EXPECT_EQ(businesses[0].stock, 9);
    EXPECT_EQ(market.getStock("小麦"), 9);
}

TEST_F(DailyCashCycleTest, UnfundedEmployer_DoesNotCreateIncome) {
    people[0].money = 0;
    businesses[0].money = 0;
    businesses[0].daily_production = 0;
    government.money = lender.money = 0;
    runDay();
    EXPECT_EQ(people[0].money, 0);
    EXPECT_EQ(totalMoney(), 0);
}

TEST_F(DailyCashCycleTest, Stock_CarriesOverWithoutDuplicateListings) {
    people[0].daily_income = 0;
    businesses[0].stock = 7;
    businesses[0].daily_production = 3;
    government.money = lender.money = 0;
    runDay();
    EXPECT_EQ(businesses[0].stock, 9);
    EXPECT_EQ(market.getStock("小麦"), 9);
    runDay();
    EXPECT_EQ(businesses[0].stock, 11);
    EXPECT_EQ(market.getStock("小麦"), 11);
}
