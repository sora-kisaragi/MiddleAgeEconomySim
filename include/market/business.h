#pragma once
#include <string>
#include <stdexcept>
#include <cstdint>
#include "../agent/agent.h"

struct Business : public Agent {
    std::string product;
    int32_t stock;
    int64_t price;
    int32_t workers;
    int32_t daily_production;
    float profit_margin;
    int32_t market_share;
    
    // デフォルトコンストラクタ
    Business() :
        product(""),
        stock(0),
        price(0),
        workers(0),
        daily_production(0),
        profit_margin(0.0f),
        market_share(0)
    {}
    
    // バリデーション付きコンストラクタ
    Business(const std::string& prod, int32_t initial_stock, int64_t initial_price,
            int32_t num_workers, int32_t production, float margin, int32_t share) {
        setProduct(prod);
        setStock(initial_stock);
        setPrice(initial_price);
        setWorkers(num_workers);
        setDailyProduction(production);
        setProfitMargin(margin);
        setMarketShare(share);
    }
    
    // 製品名の設定（空文字列チェック）
    void setProduct(const std::string& prod) {
        if (prod.empty()) {
            throw std::invalid_argument("Product name cannot be empty");
        }
        product = prod;
    }
    
    // 在庫数の設定（負数チェック）
    void setStock(int32_t amount) {
        if (amount < 0) {
            throw std::invalid_argument("Stock cannot be negative");
        }
        stock = amount;
    }
    
    // 価格の設定（負数チェック）
    void setPrice(int64_t new_price) {
        if (new_price < 0) {
            throw std::invalid_argument("Price cannot be negative");
        }
        price = new_price;
    }
    
    // 従業員数の設定（範囲チェック）
    void setWorkers(int32_t count) {
        if (count < 0) {
            throw std::invalid_argument("Number of workers cannot be negative");
        }
        workers = count;
    }
    
    // 日次生産量の設定（範囲チェック）
    void setDailyProduction(int32_t amount) {
        if (amount < 0) {
            throw std::invalid_argument("Daily production cannot be negative");
        }
        daily_production = amount;
    }
    
    // 利益率の設定（範囲チェック: 0.0 - 1.0）
    void setProfitMargin(float margin) {
        if (margin < 0.0f || margin > 1.0f) {
            throw std::out_of_range("Profit margin must be between 0.0 and 1.0");
        }
        profit_margin = margin;
    }
    
    // 市場シェアの設定（範囲チェック: 0 - 100）
    void setMarketShare(int32_t share) {
        if (share < 0 || share > 100) {
            throw std::out_of_range("Market share must be between 0 and 100");
        }
        market_share = share;
    }
    
    // 生産実行（在庫の更新）
    void produce() {
        if (workers <= 0) {
            throw std::runtime_error("Cannot produce without workers");
        }
        
        // オーバーフローチェック
        if (stock > std::numeric_limits<int32_t>::max() - daily_production) {
            throw std::overflow_error("Production would cause stock overflow");
        }
        
        stock += daily_production;
    }
    
    // 販売処理
    void sell(int32_t quantity) {
        if (quantity <= 0) {
            throw std::invalid_argument("Quantity to sell must be positive");
        }
        if (quantity > stock) {
            throw std::runtime_error("Not enough stock to sell");
        }
        
        // 売上金額の計算（オーバーフローチェック）
        int64_t revenue = static_cast<int64_t>(quantity) * price;
        if (revenue < 0) {  // オーバーフローチェック
            throw std::overflow_error("Sale would cause revenue overflow");
        }
        
        stock -= quantity;
        addMoney(revenue);  // Agent::addMoneyを使用（既存のオーバーフロー保護付き）
    }
    
    // 従業員の給与支払い
    void payWorkers(int64_t per_worker_salary) {
        if (per_worker_salary <= 0) {
            throw std::invalid_argument("Salary must be positive");
        }
        
        // 総給与の計算（オーバーフローチェック）
        if (per_worker_salary > std::numeric_limits<int64_t>::max() / workers) {
            throw std::overflow_error("Total salary would overflow");
        }
        
        int64_t total_salary = per_worker_salary * workers;
        if (total_salary > money) {
            throw std::runtime_error("Not enough money to pay workers");
        }
        
        addMoney(-total_salary);
    }
};
