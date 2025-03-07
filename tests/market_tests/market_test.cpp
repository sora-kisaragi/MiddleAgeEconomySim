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
        market->addProduct("grain", 100); // 初期価格100
        market->addStock("grain", 1000);  // 初期在庫1000
        
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
    // 需要が供給を上回る状況を作る
    market->addDemand("grain", 1500);
    market->updatePrice("grain");
    
    EXPECT_GT(market->getPrice("grain"), 100); // 価格上昇を確認
}

TEST_F(MarketTest, UpdatePrice_LowDemand) {
    // 需要が供給を下回る状況を作る
    market->addDemand("grain", 500);
    market->updatePrice("grain");
    
    EXPECT_LT(market->getPrice("grain"), 100); // 価格下落を確認
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
    float initial_volatility = market->price_volatility;
    
    // 大幅な需要超過を作り出す
    market->addStock("grain", 100);  // 供給を少なめに
    market->addDemand("grain", 1000); // 需要を大きく
    market->updatePrice("grain");
    
    // 価格変動性が増加していることを確認
    EXPECT_GT(market->price_volatility, initial_volatility);
}

TEST_F(MarketTest, MarketHistory) {
    int initial_transactions = market->demand_history["grain"].size();
    
    market->transact(buyer, seller, "grain", 10);
    
    EXPECT_GT(market->demand_history["grain"].size(), initial_transactions);
    EXPECT_GT(market->supply_history["grain"].size(), initial_transactions);
}
