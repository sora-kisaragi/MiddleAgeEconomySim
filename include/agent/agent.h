#pragma once
#include <vector>
#include <stdexcept>
#include <limits>
#include <cstdint>
#include "../market/transaction.h"

struct Agent {
    int32_t id;
    int64_t money;  // 大きな取引に対応するため64ビットに
    std::vector<Transaction*> history;
    
    // デフォルトコンストラクタ
    Agent() : 
        id(0), 
        money(0)
    {}
    
    // コピーコンストラクタ
    Agent(const Agent& other) : id(other.id), money(other.money) {
        // 取引履歴の深いコピー
        for (const auto* trans : other.history) {
            if (trans) {
                Transaction* new_trans = new Transaction(*trans);
                history.push_back(new_trans);
            }
        }
    }
    
    // ムーブコンストラクタ
    Agent(Agent&& other) noexcept 
        : id(other.id), money(other.money), history(std::move(other.history)) {
        other.id = 0;
        other.money = 0;
    }
    
    // デストラクタ
    virtual ~Agent() {
        clearHistory();
    }
    
    // コピー代入演算子
    Agent& operator=(const Agent& other) {
        if (this != &other) {
            clearHistory();
            id = other.id;
            money = other.money;
            // 取引履歴の深いコピー
            for (const auto* trans : other.history) {
                if (trans) {
                    Transaction* new_trans = new Transaction(*trans);
                    history.push_back(new_trans);
                }
            }
        }
        return *this;
    }
    
    // ムーブ代入演算子
    Agent& operator=(Agent&& other) noexcept {
        if (this != &other) {
            clearHistory();
            id = other.id;
            money = other.money;
            history = std::move(other.history);
            other.id = 0;
            other.money = 0;
        }
        return *this;
    }
    
    // お金の追加（オーバーフロー防止）
    void addMoney(int64_t amount) {
        // 最小値を直接追加しようとした場合
        if (amount == std::numeric_limits<int64_t>::min()) {
            throw std::underflow_error("Money amount would underflow");
        }
        
        // 正の値の追加時のオーバーフローチェック
        if (amount > 0 && money > std::numeric_limits<int64_t>::max() - amount) {
            throw std::overflow_error("Money amount would overflow");
        }
        
        // 負の値の追加時のアンダーフローチェック
        if (amount < 0 && money < std::numeric_limits<int64_t>::min() - amount) {
            throw std::underflow_error("Money amount would underflow");
        }
        
        money += amount;
    }
    
    // 取引履歴をクリア
    void clearHistory() {
        for (auto* trans : history) {
            delete trans;
        }
        history.clear();
    }
    
    // 取引履歴に安全に追加
    void addTransaction(Transaction* transaction) {
        if (!transaction) {
            throw std::invalid_argument("Cannot add null transaction");
        }
        history.push_back(transaction);
    }
    
protected:
    // 派生クラスでの利用のため、id設定を保護
    void setId(int32_t newId) {
        if (newId < 0) {
            throw std::invalid_argument("ID cannot be negative");
        }
        id = newId;
    }
};