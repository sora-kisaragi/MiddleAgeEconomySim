#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "agent.h"

// 健康状態を表す列挙型
enum class HealthStatus {
    DEAD = 0,
    SICK = 1,
    HEALTHY = 2
};

// 犯罪傾向を表す列挙型
enum class CrimeTendency {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2
};

struct Person : public Agent {
    std::string name;
    std::string job;
    int32_t daily_income;
    int32_t daily_expense;
    std::vector<std::string> inventory;
    
    HealthStatus health_status;
    CrimeTendency crime_tendency;
    
    int32_t satisfaction;     // 満足度（0-100）
    int32_t risk_tolerance;   // リスク選好度（0-100）
    
    // デフォルトコンストラクタ
    Person() : 
        name(""), 
        job(""), 
        daily_income(0), 
        daily_expense(0),
        health_status(HealthStatus::HEALTHY),
        crime_tendency(CrimeTendency::LOW),
        satisfaction(50),
        risk_tolerance(50)
    {}
    
    // 値の範囲チェック付きのセッター
    void setSatisfaction(int32_t value) {
        if (value < 0 || value > 100) {
            throw std::out_of_range("Satisfaction must be between 0 and 100");
        }
        satisfaction = value;
    }
    
    void setRiskTolerance(int32_t value) {
        if (value < 0 || value > 100) {
            throw std::out_of_range("Risk tolerance must be between 0 and 100");
        }
        risk_tolerance = value;
    }
    
    void setDailyIncome(int32_t value) {
        if (value < 0) {
            throw std::invalid_argument("Daily income cannot be negative");
        }
        daily_income = value;
    }
    
    void setDailyExpense(int32_t value) {
        if (value < 0) {
            throw std::invalid_argument("Daily expense cannot be negative");
        }
        daily_expense = value;
    }
    
    void setHealthStatus(HealthStatus status) {
        health_status = status;
    }
    
    void setCrimeTendency(CrimeTendency tendency) {
        crime_tendency = tendency;
    }
    
    // アイテムの安全な追加
    void addInventoryItem(const std::string& item) {
        if (item.empty()) {
            throw std::invalid_argument("Cannot add empty item to inventory");
        }
        inventory.push_back(item);
    }
    
    // アイテムの安全な削除
    bool removeInventoryItem(const std::string& item) {
        auto it = std::find(inventory.begin(), inventory.end(), item);
        if (it != inventory.end()) {
            inventory.erase(it);
            return true;
        }
        return false;
    }
    
    // 日次更新（収入と支出の処理）
    void updateDaily() {
        addMoney(daily_income);  // Agent::addMoneyを使用（オーバーフロー保護付き）
        addMoney(-daily_expense);
    }
};