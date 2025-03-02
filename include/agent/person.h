#pragma once
#include <string>
#include <vector>

struct Person {
    // 基本情報
    int id;
    std::string name;
    int money;
    
    // 職業と経済状態
    std::string job;
    int daily_income;
    int daily_expense;
    
    // 所有物
    std::vector<std::string> inventory;
    
    // 状態属性
    int health_status;    // 健康状態（0: 死亡, 1: 病気, 2: 健康）
    int crime_tendency;   // 犯罪傾向（0: 低, 1: 中, 2: 高）
    
    // 行動決定要素
    int satisfaction;     // 満足度（0-100）
    int risk_tolerance;   // リスク選好度（0-100）
    
    // デフォルトコンストラクタ
    Person() : 
        id(0), 
        name(""), 
        money(0), 
        job(""), 
        daily_income(0), 
        daily_expense(0), 
        health_status(2),  // 初期状態は健康
        crime_tendency(0),  // 初期状態は犯罪傾向低
        satisfaction(50),   // 中程度の満足度
        risk_tolerance(50)  // 中程度のリスク選好度
    {}
};