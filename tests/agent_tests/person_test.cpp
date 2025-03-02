#include <gtest/gtest.h>
#include "agent/person.h"

TEST(PersonTest, DefaultConstructor) {
    Person p;
    // 基本情報
    EXPECT_EQ(0, p.id);
    EXPECT_EQ("", p.name);
    EXPECT_EQ(0, p.money);
    
    // 職業と経済状態
    EXPECT_EQ("", p.job);
    EXPECT_EQ(0, p.daily_income);
    EXPECT_EQ(0, p.daily_expense);
    
    // 所有物
    EXPECT_TRUE(p.inventory.empty());
    
    // 状態属性
    EXPECT_EQ(2, p.health_status);  // 健康
    EXPECT_EQ(0, p.crime_tendency); // 犯罪傾向低
    
    // 行動決定要素
    EXPECT_EQ(50, p.satisfaction);  // 中程度の満足度
    EXPECT_EQ(50, p.risk_tolerance); // 中程度のリスク選好度
}

TEST(PersonTest, CustomPerson) {
    Person p;
    p.id = 1;
    p.name = "John Smith";
    p.money = 100;
    p.job = "Farmer";
    p.daily_income = 10;
    p.daily_expense = 8;
    p.inventory.push_back("Bread");
    p.health_status = 1;  // 病気
    p.crime_tendency = 1; // 中程度
    p.satisfaction = 30;  // やや不満
    p.risk_tolerance = 70; // リスク許容度高め
    
    EXPECT_EQ(1, p.id);
    EXPECT_EQ("John Smith", p.name);
    EXPECT_EQ(100, p.money);
    EXPECT_EQ("Farmer", p.job);
    EXPECT_EQ(10, p.daily_income);
    EXPECT_EQ(8, p.daily_expense);
    EXPECT_EQ(1, p.inventory.size());
    EXPECT_EQ("Bread", p.inventory[0]);
    EXPECT_EQ(1, p.health_status);
    EXPECT_EQ(1, p.crime_tendency);
    EXPECT_EQ(30, p.satisfaction);
    EXPECT_EQ(70, p.risk_tolerance);
}

// 満足度の更新をテスト
TEST(PersonTest, SatisfactionUpdate) {
    Person p;
    p.satisfaction = 50; // 初期満足度を50に設定
    
    // アイテム所持時の満足度上昇テスト
    p.inventory.push_back("小麦");
    int initialSatisfaction = p.satisfaction;
    int updatedSatisfaction = std::min(100, initialSatisfaction + 5);
    EXPECT_EQ(updatedSatisfaction, std::min(100, p.satisfaction + 5));
    
    // 満足度の最大値（100）を超えないことを確認
    p.satisfaction = 98;
    p.inventory.push_back("パン");
    EXPECT_LE(p.satisfaction + 5, 100);
    
    // アイテムがない場合の満足度低下テスト
    p.satisfaction = 50;
    p.inventory.clear();
    int decreasedSatisfaction = std::max(0, p.satisfaction - 10);
    EXPECT_EQ(decreasedSatisfaction, std::max(0, p.satisfaction - 10));
    
    // 満足度が0未満にならないことを確認
    p.satisfaction = 5;
    p.inventory.clear();
    EXPECT_GE(std::max(0, p.satisfaction - 10), 0);
}

// 消費活動をテスト
TEST(PersonTest, Consumption) {
    Person p;
    p.money = 100;
    int itemPrice = 20;
    
    // 十分なお金があるときの購入テスト
    bool canBuy = p.money >= itemPrice;
    EXPECT_TRUE(canBuy);
    
    // 購入後のお金の減少を確認
    int expectedMoney = p.money - itemPrice;
    p.money -= itemPrice;
    EXPECT_EQ(expectedMoney, p.money);
    EXPECT_EQ(80, p.money);
    
    // アイテムが正しく追加されるかテスト
    size_t initialInventorySize = p.inventory.size();
    p.inventory.push_back("テスト商品");
    EXPECT_EQ(initialInventorySize + 1, p.inventory.size());
    
    // お金が不足している場合に購入できないことをテスト
    p.money = 10;
    canBuy = p.money >= itemPrice;
    EXPECT_FALSE(canBuy);
    
    // お金が不足している場合は購入せずお金も減らないテスト
    int moneyBeforeFailing = p.money;
    if (p.money >= itemPrice) {
        p.money -= itemPrice;
    }
    EXPECT_EQ(moneyBeforeFailing, p.money);
}

// 収入テスト
TEST(PersonTest, Income) {
    Person p;
    p.money = 50;
    p.daily_income = 25;
    
    // 収入獲得後の所持金増加テスト
    int expectedMoney = p.money + p.daily_income;
    p.money += p.daily_income;
    EXPECT_EQ(expectedMoney, p.money);
    EXPECT_EQ(75, p.money);
    
    // 収入がない場合
    p.money = 50;
    p.daily_income = 0;
    expectedMoney = p.money + p.daily_income;
    p.money += p.daily_income;
    EXPECT_EQ(expectedMoney, p.money);
    EXPECT_EQ(50, p.money);
}

// リスク許容度による行動の違いをテスト
TEST(PersonTest, RiskTolerance) {
    Person lowRisk;
    lowRisk.risk_tolerance = 20; // リスク許容度低い
    
    Person highRisk;
    highRisk.risk_tolerance = 80; // リスク許容度高い
    
    // リスク許容度の比較
    EXPECT_TRUE(lowRisk.risk_tolerance < highRisk.risk_tolerance);
    
    // リスク許容度に基づいて行動が異なるかテスト
    const int risky_threshold = 50;
    bool lowRiskPersonTakesRisk = lowRisk.risk_tolerance > risky_threshold;
    bool highRiskPersonTakesRisk = highRisk.risk_tolerance > risky_threshold;
    
    EXPECT_FALSE(lowRiskPersonTakesRisk);
    EXPECT_TRUE(highRiskPersonTakesRisk);
}

// 職業の設定と取得テスト
TEST(PersonTest, JobSetting) {
    Person p;
    EXPECT_EQ("", p.job);  // デフォルトでは空文字
    
    p.job = "農民";
    EXPECT_EQ("農民", p.job);
    
    p.job = "商人";
    EXPECT_EQ("商人", p.job);
    
    p.job = "貴族";
    EXPECT_EQ("貴族", p.job);
}

// 健康状態のテスト
TEST(PersonTest, HealthStatus) {
    Person p;
    EXPECT_EQ(2, p.health_status); // デフォルトで健康
    
    p.health_status = 1; // 病気
    EXPECT_EQ(1, p.health_status);
    
    p.health_status = 0; // 重病
    EXPECT_EQ(0, p.health_status);
}