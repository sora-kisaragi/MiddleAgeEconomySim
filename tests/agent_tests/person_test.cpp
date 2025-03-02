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