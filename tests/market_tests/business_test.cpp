#include "../owned_market_fixture.h"
#include <gtest/gtest.h>
#include "../../include/market/business.h"
#include <limits>

// Business構造体の基本テスト
TEST(BusinessTest, DefaultConstructor) {
    Business business;
    
    // デフォルト値の確認
    EXPECT_EQ(business.product, "");
    EXPECT_EQ(business.stock, 0);
    EXPECT_EQ(business.price, 0);
    EXPECT_EQ(business.workers, 0);
    EXPECT_EQ(business.daily_production, 0);
    EXPECT_FLOAT_EQ(business.profit_margin, 0.0f);
    EXPECT_EQ(business.market_share, 0);
}

// バリデーション付きコンストラクタのテスト
TEST(BusinessTest, ValidatedConstructor) {
    EXPECT_NO_THROW({
        Business business("Product1", 100, 50, 10, 20, 0.3f, 25);
        EXPECT_EQ(business.product, "Product1");
        EXPECT_EQ(business.stock, 100);
        EXPECT_EQ(business.price, 50);
        EXPECT_EQ(business.workers, 10);
        EXPECT_EQ(business.daily_production, 20);
        EXPECT_FLOAT_EQ(business.profit_margin, 0.3f);
        EXPECT_EQ(business.market_share, 25);
    });
}

// 値の設定と検証のテスト
TEST(BusinessTest, SettersValidation) {
    Business business;
    
    // 製品名の設定
    EXPECT_NO_THROW(business.setProduct("Test Product"));
    EXPECT_THROW(business.setProduct(""), std::invalid_argument);
    
    // 在庫の設定
    EXPECT_NO_THROW(business.setStock(100));
    EXPECT_THROW(business.setStock(-1), std::invalid_argument);
    
    // 価格の設定
    EXPECT_NO_THROW(business.setPrice(50));
    EXPECT_THROW(business.setPrice(-1), std::invalid_argument);
    
    // 従業員数の設定
    EXPECT_NO_THROW(business.setWorkers(10));
    EXPECT_THROW(business.setWorkers(-1), std::invalid_argument);
    
    // 日次生産量の設定
    EXPECT_NO_THROW(business.setDailyProduction(20));
    EXPECT_THROW(business.setDailyProduction(-1), std::invalid_argument);
    
    // 利益率の設定
    EXPECT_NO_THROW(business.setProfitMargin(0.3f));
    EXPECT_THROW(business.setProfitMargin(-0.1f), std::out_of_range);
    EXPECT_THROW(business.setProfitMargin(1.1f), std::out_of_range);
    
    // 市場シェアの設定
    EXPECT_NO_THROW(business.setMarketShare(25));
    EXPECT_THROW(business.setMarketShare(-1), std::out_of_range);
    EXPECT_THROW(business.setMarketShare(101), std::out_of_range);
}

// 生産機能のテスト
TEST(BusinessTest, ProductionRequiresLedger) {
    Business business;
    business.workers = 5; business.daily_production = 10;
    EXPECT_THROW(business.produce(), std::logic_error);
    EXPECT_EQ(business.stock, 0);
}

TEST(BusinessTest, SaleRequiresCounterparty) {
    Business business("Test",100,50,1,1,0.0f,0);
    for (int quantity : {0,-1,50,101}) {
        EXPECT_THROW(business.sell(quantity), std::logic_error);
        EXPECT_EQ(business.stock,100);
        EXPECT_EQ(business.money,0);
    }
}

// 給与支払い機能のテスト
TEST(BusinessTest, WorkerPayment) {
    Business business;
    business.setWorkers(10);
    business.addMoney(10000);
    
    // 不正な給与での支払い試行
    EXPECT_THROW(business.payWorkers(0), std::invalid_argument);
    EXPECT_THROW(business.payWorkers(-1), std::invalid_argument);
    
    // 資金不足での支払い試行
    EXPECT_THROW(business.payWorkers(2000), std::runtime_error);
    
    // 正常な支払い
    EXPECT_NO_THROW(business.payWorkers(500));
    EXPECT_EQ(business.money, 5000);  // 10000 - (500 * 10)
    
    // 大量支払い時のオーバーフローチェック
    Business large_business;
    large_business.setWorkers(std::numeric_limits<int32_t>::max());
    EXPECT_THROW(large_business.payWorkers(std::numeric_limits<int64_t>::max() / 1000), std::overflow_error);
}
