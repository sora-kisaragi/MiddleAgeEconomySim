#include <gtest/gtest.h>
#include <vector>
#include "agent/person.h"
#include "agent/government.h"
#include "agent/loan_provider.h"
#include "market/business.h"
#include "market/market.h"
#include "system/trade_route.h"

// Integration test for the economic simulation system
class EconomicSimulationIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize market
        market = new Market();
        market->setPriceVolatility(0.1f);
        
        // Initialize government
        government = new Government();
        government->money = 1000;
        government->approval_rating = 75.0f;
        
        // Initialize loan provider
        loan_provider = new LoanProvider();
        loan_provider->money = 5000;
        
        // Create people
        Person farmer;
        farmer.name = "農夫";
        farmer.job = "農業";
        farmer.setDailyIncome(50);
        farmer.setDailyExpense(30);
        farmer.money = 100;
        people.push_back(farmer);
        
        Person merchant;
        merchant.name = "商人";
        merchant.job = "商売";
        merchant.setDailyIncome(80);
        merchant.setDailyExpense(40);
        merchant.money = 200;
        people.push_back(merchant);
        
        // Create businesses
        Business farm;
        farm.product = "小麦";
        farm.daily_production = 10;
        farm.price = 5;
        farm.stock = 20;
        farm.money = 500;
        businesses.push_back(farm);
        
        Business bakery;
        bakery.product = "パン";
        bakery.daily_production = 5;
        bakery.price = 10;
        bakery.stock = 15;
        bakery.money = 300;
        businesses.push_back(bakery);
        
        // Setup trade route
        TradeRoute mainRoute;
        mainRoute.from_location_id = 1;
        mainRoute.to_location_id = 2;
        mainRoute.goods["小麦"] = 10;
        mainRoute.goods["パン"] = 5;
        mainRoute.travel_time = 3;
        trade_routes.push_back(mainRoute);
    }

    void TearDown() override {
        delete market;
        delete government;
        delete loan_provider;
    }

    Market* market;
    Government* government;
    LoanProvider* loan_provider;
    std::vector<Person> people;
    std::vector<Business> businesses;
    std::vector<TradeRoute> trade_routes;
};

TEST_F(EconomicSimulationIntegrationTest, BasicMarketOperations) {
    // Register products in market
    market->registerProduct("小麦", 5);
    market->registerProduct("パン", 10);
    
    // Add initial stock
    market->sell("小麦", 50, 5);
    market->sell("パン", 20, 10);
    
    EXPECT_EQ(market->getStock("小麦"), 50);
    EXPECT_EQ(market->getStock("パン"), 20);
    EXPECT_EQ(market->getPrice("小麦"), 5);
    EXPECT_EQ(market->getPrice("パン"), 10);
}

TEST_F(EconomicSimulationIntegrationTest, PersonBusinessInteraction) {
    // Set up market
    market->registerProduct("小麦", 5);
    market->sell("小麦", 50, 5);
    
    Person& farmer = people[0];
    Business& farm = businesses[0];
    
    // Person can interact with business
    EXPECT_TRUE(farmer.canInteractWith(&farm));
    
    // Person buys from business via market transaction
    int initial_farmer_money = farmer.money;
    int initial_farm_money = farm.money;
    
    bool transaction_success = market->transact(&farmer, &farm, "小麦", 5);
    
    EXPECT_TRUE(transaction_success);
    EXPECT_LT(farmer.money, initial_farmer_money);
    EXPECT_GT(farm.money, initial_farm_money);
}

TEST_F(EconomicSimulationIntegrationTest, GovernmentTaxCollection) {
    Person& merchant = people[1];
    merchant.money = 500; // Ensure sufficient funds for tax
    
    int initial_government_money = government->money;
    int initial_merchant_money = merchant.money;
    
    bool tax_collected = government->collectTax(&merchant);
    
    EXPECT_TRUE(tax_collected);
    EXPECT_GT(government->money, initial_government_money);
    EXPECT_LT(merchant.money, initial_merchant_money);
}

TEST_F(EconomicSimulationIntegrationTest, LoanProviderService) {
    Person& farmer = people[0];
    farmer.money = 50; // Low money to need a loan
    
    int initial_loan_provider_money = loan_provider->money;
    int initial_farmer_money = farmer.money;
    
    bool loan_granted = loan_provider->provideLoan(&farmer, 200);
    
    EXPECT_TRUE(loan_granted);
    EXPECT_LT(loan_provider->money, initial_loan_provider_money);
    EXPECT_GT(farmer.money, initial_farmer_money);
}

TEST_F(EconomicSimulationIntegrationTest, AgentToAgentDirectTrade) {
    Person& farmer = people[0];
    Person& merchant = people[1];
    
    farmer.money = 200;
    merchant.money = 300;
    
    // Direct trade between agents
    bool trade_success = farmer.directTrade(&merchant, "tools", 2, 50);
    
    EXPECT_TRUE(trade_success);
    EXPECT_EQ(farmer.money, 100); // 200 - (2 * 50)
    EXPECT_EQ(merchant.money, 400); // 300 + (2 * 50)
}

TEST_F(EconomicSimulationIntegrationTest, MultipleAgentEconomicCycle) {
    // Setup market
    market->registerProduct("小麦", 5);
    market->sell("小麦", 100, 5);
    
    // Record initial state
    int total_money_before = 0;
    for (const auto& person : people) {
        total_money_before += person.money;
    }
    for (const auto& business : businesses) {
        total_money_before += business.money;
    }
    total_money_before += government->money + loan_provider->money;
    
    // Simulate some economic activity
    
    // 1. Business production (adds value to economy)
    for (auto& business : businesses) {
        business.stock += business.daily_production;
    }
    
    // 2. People earn income
    for (auto& person : people) {
        person.money += person.daily_income;
    }
    
    // 3. Market transactions
    for (auto& person : people) {
        if (person.money >= 10 && market->getStock("小麦") > 0) {
            try {
                int cost = market->buy("小麦", 1);
                person.money -= cost;
            } catch (const std::exception&) {
                // Transaction failed, continue
            }
        }
    }
    
    // 4. Government tax collection
    for (auto& person : people) {
        if (person.money > 100) {
            government->collectTax(&person);
        }
    }
    
    // Economy should still function after multiple operations
    EXPECT_GT(market->getStock("小麦"), 0);
    EXPECT_GT(government->money, 1000); // Should have collected some tax
    
    // Verify that people still have reasonable money amounts
    for (const auto& person : people) {
        EXPECT_GE(person.money, 0);
    }
}

TEST_F(EconomicSimulationIntegrationTest, TradeRouteExistence) {
    EXPECT_FALSE(trade_routes.empty());
    EXPECT_EQ(trade_routes[0].from_location_id, 1);
    EXPECT_EQ(trade_routes[0].to_location_id, 2);
    EXPECT_GT(trade_routes[0].travel_time, 0);
}

TEST_F(EconomicSimulationIntegrationTest, SystemStabilityAfterManyOperations) {
    // Setup market
    market->registerProduct("小麦", 5);
    market->sell("小麦", 1000, 5);
    
    // Run many operations to test system stability
    for (int day = 0; day < 10; ++day) {
        // Daily production
        for (auto& business : businesses) {
            business.stock += business.daily_production;
            market->sell(business.product, business.daily_production, business.price);
        }
        
        // Daily income
        for (auto& person : people) {
            person.money += person.daily_income;
        }
        
        // Market purchases
        for (auto& person : people) {
            if (person.money >= market->getPrice("小麦") && market->getStock("小麦") > 0) {
                try {
                    int cost = market->buy("小麦", 1);
                    person.money -= cost;
                } catch (const std::exception&) {
                    // Skip if transaction fails
                }
            }
        }
        
        // Government activities
        for (auto& person : people) {
            if (person.money > 100) {
                government->collectTax(&person);
            }
        }
        
        // Verify system integrity each day
        EXPECT_GT(market->getStock("小麦"), 0);
        EXPECT_GT(government->money, 0);
        for (const auto& person : people) {
            EXPECT_GE(person.money, 0);
        }
    }
    
    // System should still be stable after 10 days
    EXPECT_GT(government->money, 1000); // Should have collected taxes
    EXPECT_LT(market->getPriceVolatility(), 1.0f); // Volatility should be reasonable
}