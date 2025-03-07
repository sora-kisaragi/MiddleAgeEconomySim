#include <gtest/gtest.h>
#include "../../include/agent/person.h"

// Person構造体の基本テスト
TEST(PersonTest, Creation) {
    Person person;
    
    // Agent部分の初期化を確認
    EXPECT_EQ(person.id, 0);
    EXPECT_EQ(person.money, 0);
    
    // Person固有のフィールドが正しく初期化されているか確認
    EXPECT_EQ(person.name, "");
    EXPECT_EQ(person.job, "");
    EXPECT_EQ(person.daily_income, 0);
    EXPECT_EQ(person.daily_expense, 0);
    EXPECT_TRUE(person.inventory.empty());
    EXPECT_EQ(person.health_status, HealthStatus::HEALTHY);
    EXPECT_EQ(person.crime_tendency, CrimeTendency::LOW);
    EXPECT_EQ(person.satisfaction, 50);
    EXPECT_EQ(person.risk_tolerance, 50);
}

// Personの情報設定テスト
TEST(PersonTest, SetPersonInfo) {
    Person person;
    
    // 個人情報を設定
    person.id = 101;
    person.name = "John Doe";
    person.job = "Farmer";
    person.money = 500;
    
    EXPECT_EQ(person.id, 101);
    EXPECT_EQ(person.name, "John Doe");
    EXPECT_EQ(person.job, "Farmer");
    EXPECT_EQ(person.money, 500);
}

// 経済活動の設定テスト
TEST(PersonTest, EconomicActivity) {
    Person person;
    
    // 経済活動パラメータを設定
    person.setDailyIncome(50);
    person.setDailyExpense(30);
    person.setSatisfaction(60);
    person.setRiskTolerance(40);
    
    EXPECT_EQ(person.daily_income, 50);
    EXPECT_EQ(person.daily_expense, 30);
    EXPECT_EQ(person.satisfaction, 60);
    EXPECT_EQ(person.risk_tolerance, 40);
    
    // 日次の収支が正しいことを確認
    int daily_balance = person.daily_income - person.daily_expense;
    EXPECT_EQ(daily_balance, 20);
}

// 所持品の管理テスト
TEST(PersonTest, InventoryManagement) {
    Person person;
    
    // 空のアイテムの追加を試みる
    EXPECT_THROW(person.addInventoryItem(""), std::invalid_argument);
    
    // 正常なアイテムの追加
    EXPECT_NO_THROW(person.addInventoryItem("Bread"));
    EXPECT_NO_THROW(person.addInventoryItem("Tool"));
    EXPECT_EQ(person.inventory.size(), 2);
    
    // アイテムの削除
    EXPECT_TRUE(person.removeInventoryItem("Bread"));
    EXPECT_EQ(person.inventory.size(), 1);
    
    // 存在しないアイテムの削除を試みる
    EXPECT_FALSE(person.removeInventoryItem("NonExistentItem"));
}

// 健康状態の変更テスト
TEST(PersonTest, HealthStatusChange) {
    Person person;
    
    // 初期状態は健康
    EXPECT_EQ(person.health_status, HealthStatus::HEALTHY);
    
    // 病気に変更
    person.setHealthStatus(HealthStatus::SICK);
    EXPECT_EQ(person.health_status, HealthStatus::SICK);
    
    // 死亡に変更
    person.setHealthStatus(HealthStatus::DEAD);
    EXPECT_EQ(person.health_status, HealthStatus::DEAD);
}

// 犯罪傾向の変更テスト
TEST(PersonTest, CrimeTendencyChange) {
    Person person;
    
    // 初期状態は低
    EXPECT_EQ(person.crime_tendency, CrimeTendency::LOW);
    
    // 中程度に変更
    person.setCrimeTendency(CrimeTendency::MEDIUM);
    EXPECT_EQ(person.crime_tendency, CrimeTendency::MEDIUM);
    
    // 高に変更
    person.setCrimeTendency(CrimeTendency::HIGH);
    EXPECT_EQ(person.crime_tendency, CrimeTendency::HIGH);
}

// 基本的な継承チェック
TEST(PersonTest, InheritanceCheck) {
    Person person;
    Agent* agent = &person;  // 基底クラスのポインタに代入可能であることを確認
    
    agent->id = 202;
    agent->money = 1000;
    
    EXPECT_EQ(person.id, 202);
    EXPECT_EQ(person.money, 1000);
}

// 範囲チェックのテスト
TEST(PersonTest, RangeValidation) {
    Person person;
    
    // 満足度の範囲チェック
    EXPECT_NO_THROW(person.setSatisfaction(0));
    EXPECT_NO_THROW(person.setSatisfaction(100));
    EXPECT_THROW(person.setSatisfaction(-1), std::out_of_range);
    EXPECT_THROW(person.setSatisfaction(101), std::out_of_range);
    
    // リスク選好度の範囲チェック
    EXPECT_NO_THROW(person.setRiskTolerance(0));
    EXPECT_NO_THROW(person.setRiskTolerance(100));
    EXPECT_THROW(person.setRiskTolerance(-1), std::out_of_range);
    EXPECT_THROW(person.setRiskTolerance(101), std::out_of_range);
    
    // 日次収入と支出の範囲チェック
    EXPECT_NO_THROW(person.setDailyIncome(0));
    EXPECT_NO_THROW(person.setDailyExpense(0));
    EXPECT_THROW(person.setDailyIncome(-1), std::invalid_argument);
    EXPECT_THROW(person.setDailyExpense(-1), std::invalid_argument);
}

// 日次更新のテスト
TEST(PersonTest, DailyUpdate) {
    Person person;
    person.setDailyIncome(100);
    person.setDailyExpense(30);
    
    // 日次更新の実行
    person.updateDaily();
    
    // 収支が正しく反映されているか確認
    EXPECT_EQ(person.money, 70);  // 100(収入) - 30(支出) = 70
    
    // 複数日のテスト
    person.updateDaily();
    EXPECT_EQ(person.money, 140);  // 70 + (100 - 30) = 140
}

// 金銭管理のテスト
TEST(PersonTest, MoneyManagement) {
    Person person;
    
    // 正常な金銭の追加
    person.addMoney(1000);
    EXPECT_EQ(person.money, 1000);
    
    // 負の金額の処理（支出）
    person.addMoney(-500);
    EXPECT_EQ(person.money, 500);
}

// ステータス管理のテスト
TEST(PersonTest, StatusManagement) {
    Person person;
    
    // 健康状態の設定
    person.setHealthStatus(HealthStatus::SICK);
    EXPECT_EQ(person.health_status, HealthStatus::SICK);
    
    // 犯罪傾向の設定
    person.setCrimeTendency(CrimeTendency::HIGH);
    EXPECT_EQ(person.crime_tendency, CrimeTendency::HIGH);
}

// 満足度管理のテスト
TEST(PersonTest, SatisfactionManagement) {
    Person person;
    
    // 正常な満足度の設定
    person.setSatisfaction(75);
    EXPECT_EQ(person.satisfaction, 75);
    
    // 範囲外の満足度を設定
    EXPECT_THROW(person.setSatisfaction(-1), std::out_of_range);
    EXPECT_THROW(person.setSatisfaction(101), std::out_of_range);
}