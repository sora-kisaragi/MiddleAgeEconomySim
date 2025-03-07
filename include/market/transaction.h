#pragma once
#include <string>
#include <ctime>
#include <stdexcept>
#include <cstdint>

struct Transaction {
    int32_t transaction_id;
    int32_t seller_id;
    int32_t buyer_id;
    std::string item;
    int32_t quantity;
    int64_t price;  // 大きな取引に対応するため64ビットに
    time_t timestamp;
    
    // デフォルトコンストラクタ
    Transaction() : 
        transaction_id(0), 
        seller_id(0), 
        buyer_id(0), 
        item(""), 
        quantity(0), 
        price(0), 
        timestamp(std::time(nullptr))  // 現在時刻をデフォルトに
    {}
    
    // 完全コンストラクタ
    Transaction(int32_t t_id, int32_t s_id, int32_t b_id, 
               const std::string& t_item, int32_t qty, int64_t prc) 
        : transaction_id(t_id)
        , seller_id(s_id)
        , buyer_id(b_id)
        , item(t_item)
        , quantity(qty)
        , price(prc)
        , timestamp(std::time(nullptr)) 
    {
        validate();
    }
    
    // バリデーション
    void validate() const {
        if (transaction_id < 0) {
            throw std::invalid_argument("Transaction ID cannot be negative");
        }
        if (seller_id < 0) {
            throw std::invalid_argument("Seller ID cannot be negative");
        }
        if (buyer_id < 0) {
            throw std::invalid_argument("Buyer ID cannot be negative");
        }
        if (item.empty()) {
            throw std::invalid_argument("Item cannot be empty");
        }
        if (quantity <= 0) {
            throw std::invalid_argument("Quantity must be positive");
        }
        if (price < 0) {
            throw std::invalid_argument("Price cannot be negative");
        }
    }
    
    // 総取引額の計算
    int64_t getTotalValue() const {
        // オーバーフロー検出
        if (price > 0 && quantity > std::numeric_limits<int64_t>::max() / price) {
            throw std::overflow_error("Total value would overflow");
        }
        return price * quantity;
    }
    
    // トランザクションの経過時間（秒）を取得
    time_t getElapsedTime() const {
        return std::time(nullptr) - timestamp;
    }
    
    // トランザクションが指定時間（秒）以上経過しているかチェック
    bool isOlderThan(time_t seconds) const {
        return getElapsedTime() >= seconds;
    }
    
    // セッターメソッド（バリデーション付き）
    void setQuantity(int32_t qty) {
        if (qty <= 0) {
            throw std::invalid_argument("Quantity must be positive");
        }
        quantity = qty;
    }
    
    void setPrice(int64_t prc) {
        if (prc < 0) {
            throw std::invalid_argument("Price cannot be negative");
        }
        price = prc;
    }
    
    void setItem(const std::string& new_item) {
        if (new_item.empty()) {
            throw std::invalid_argument("Item cannot be empty");
        }
        item = new_item;
    }
};