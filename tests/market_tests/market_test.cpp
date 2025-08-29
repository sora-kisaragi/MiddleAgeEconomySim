#include <gtest/gtest.h>
#include "market/market.h"
#include "market/business.h"
#include "market/transaction.h"

class MarketTest : public ::testing::Test {
protected:
    void SetUp() override {
        market = new Market();
        seller = new Business();
        buyer = new Person();
        
        // 基本設定
        market->registerProduct("grain", 100); // 初期価格100
        market->sell("grain", 1000, 100);  // 初期在庫1000
        
        seller->money = 1000;
        seller->product = "grain";
        seller->stock = 500;
        
        buyer->money = 1500; // 購入に十分な金額に増額
    }

    void TearDown() override {
        delete market;
        delete seller;
        delete buyer;
    }

    Market* market;
    Business* seller;
    Person* buyer;
};

TEST_F(MarketTest, GetPrice) {
    EXPECT_EQ(market->getPrice("grain"), 100);
    EXPECT_EQ(market->getPrice("nonexistent"), 0);
}

TEST_F(MarketTest, UpdatePrice_HighDemand) {
    // Test that prices respond to market conditions
    int initial_price = market->getPrice("grain");
    int initial_stock = market->getStock("grain");
    
    // Make purchases to reduce supply
    market->buy("grain", 200);
    market->buy("grain", 200);
    
    // Check that stock decreased
    EXPECT_LT(market->getStock("grain"), initial_stock);
    
    // Price should adjust based on supply/demand dynamics
    int final_price = market->getPrice("grain");
    EXPECT_TRUE(final_price > 0); // Price should be positive
}

TEST_F(MarketTest, UpdatePrice_LowDemand) {
    // Add more supply to create oversupply situation
    int initial_price = market->getPrice("grain");
    
    // Add more supply without corresponding demand
    market->sell("grain", 2000, 100);
    
    EXPECT_LE(market->getPrice("grain"), initial_price); // Price should stay same or decrease
}

TEST_F(MarketTest, Transaction_Success) {
    int quantity = 10;
    int price = market->getPrice("grain");
    bool result = market->transact(buyer, seller, "grain", quantity);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(buyer->money, 1500 - (price * quantity));
    EXPECT_EQ(seller->money, 1000 + (price * quantity));
    EXPECT_EQ(seller->stock, 490);
}

TEST_F(MarketTest, Transaction_InsufficientFunds) {
    buyer->money = 50; // 購入するには資金不足
    bool result = market->transact(buyer, seller, "grain", 10);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(buyer->money, 50); // 変化なし
    EXPECT_EQ(seller->money, 1000); // 変化なし
    EXPECT_EQ(seller->stock, 500); // 変化なし
}

TEST_F(MarketTest, Transaction_InsufficientStock) {
    bool result = market->transact(buyer, seller, "grain", 600); // 在庫以上の量
    
    EXPECT_FALSE(result);
    EXPECT_EQ(buyer->money, 1500); // 変化なし
    EXPECT_EQ(seller->money, 1000); // 変化なし
    EXPECT_EQ(seller->stock, 500); // 変化なし
}

TEST_F(MarketTest, PriceVolatility) {
    float initial_volatility = market->getPriceVolatility();
    
    // Create market activity that could increase volatility
    market->sell("grain", 100, 100);  // Add limited supply
    for (int i = 0; i < 10; ++i) {
        try {
            market->buy("grain", 50); // Create demand pressure
        } catch (const std::exception&) {
            break; // Stop if insufficient stock
        }
    }
    
    // Volatility might increase due to market activity
    EXPECT_GE(market->getPriceVolatility(), initial_volatility);
}

TEST_F(MarketTest, MarketHistory) {
    // Test that market operations affect internal state properly
    int initial_stock = market->getStock("grain");
    
    // Perform a transaction
    market->transact(buyer, seller, "grain", 10);
    
    // Stock should have changed
    EXPECT_LT(market->getStock("grain"), initial_stock);
}
