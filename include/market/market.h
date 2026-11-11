#ifndef MARKET_H
#define MARKET_H

#include <string>
#include <map>
#include <cstdint>
#include <cmath>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include "../agent/person.h"
#include "../market/business.h"

class Market {
private:
    std::map<std::string, int> stock;
    std::map<std::string, int> price;
    std::map<std::string, int64_t> daily_demand;
    std::map<std::string, int64_t> daily_supply;
    std::map<std::string, int> opening_price;
    float price_volatility;
    
    static constexpr float MAX_VOLATILITY = 2.0f;    // Cap volatility to prevent instability

public:
    Market() : price_volatility(0.1f) {}
    
    // Getter methods for encapsulated data
    float getPriceVolatility() const { return price_volatility; }
    void setPriceVolatility(float volatility) { 
        price_volatility = std::max(0.0f, std::min(volatility, MAX_VOLATILITY)); 
    }

    // Public interface methods
    int getPrice(const std::string& product) const {
        auto it = price.find(product);
        return it != price.end() ? it->second : 0;
    }

    int getStock(const std::string& product) const {
        auto it = stock.find(product);
        return it != stock.end() ? it->second : 0;
    }

    // Explicit recalculation uses the day's opening price, so repeated calls are idempotent.
    void updatePrice(const std::string& product) {
        auto initial = opening_price.find(product);
        if (initial == opening_price.end()) return;
        const int64_t supply = daily_supply[product];
        const int64_t demand = daily_demand[product];
        if (supply == 0 && demand == 0) return;

        const double change = static_cast<double>(price_volatility) *
            (static_cast<double>(demand) - supply) / std::max<int64_t>(1, supply);
        const double next_price = initial->second * (1.0 + change);
        price[product] = static_cast<int>(std::round(std::clamp(
            next_price, 1.0, static_cast<double>(std::numeric_limits<int>::max()))));
    }

    bool transact(Person* buyer, Business* seller, const std::string& product, int quantity) {
        if (!buyer || !seller || quantity <= 0) return false;
        
        // 商品が市場に存在するか確認
        auto price_it = price.find(product);
        if (price_it == price.end()) {
            addProduct(product, seller->price); // 新商品として登録
            price_it = price.find(product);
        }
        
        int total_cost = price_it->second * quantity;
        
        // 購入者の所持金と売り手の在庫を確認
        if (buyer->money < total_cost) return false;
        if (seller->stock < quantity) {
            addDemand(product, quantity);
            return false;
        }

        // 市場の在庫を確認と更新
        auto& market_stock = stock[product];
        if (market_stock < quantity) {
            // 市場の在庫が不足している場合は、売り手から補充
            addStock(product, seller->stock);
            market_stock = stock[product];
        }

        if (market_stock < quantity) {
            return false;
        }

        // 取引実行
        buyer->addMoney(-total_cost);
        seller->addMoney(total_cost);
        seller->stock -= quantity;
        market_stock -= quantity;

        // 取引履歴の更新
        addDemand(product, quantity);

        return true;
    }

    void registerProduct(const std::string& product, int initial_price) {
        addProduct(product, initial_price);
    }

    bool sell(const std::string& product, int quantity, int price) {
        if (this->price.find(product) == this->price.end()) {
            addProduct(product, price);
        }
        addStock(product, quantity);
        return true;
    }

    int buy(const std::string& product, int quantity) {
        if (quantity <= 0) {
            throw std::invalid_argument("Quantity must be positive");
        }
        auto price_it = price.find(product);
        if (price_it == price.end()) {
            throw std::invalid_argument("Product not found in market");
        }
        addDemand(product, quantity);
        if (stock[product] < quantity) {
            throw std::invalid_argument("Insufficient stock");
        }
        int total_cost = price_it->second * quantity;
        stock[product] -= quantity;
        return total_cost;
    }

    // Close the day, then carry the remaining goods into the next day's supply.
    void clearDaily() {
        for (const auto& entry : price) {
            const auto& product = entry.first;
            updatePrice(product);
            opening_price[product] = price[product];
            daily_demand[product] = 0;
            daily_supply[product] = stock[product];
        }
    }

private:
    void addProduct(const std::string& product, int initial_price) {
        price[product] = initial_price;
        opening_price[product] = initial_price;
        stock[product] = 0;
        daily_demand[product] = 0;
        daily_supply[product] = 0;
    }

    void addStock(const std::string& product, int quantity) {
        stock[product] += quantity;
        daily_supply[product] += quantity;
    }

    void addDemand(const std::string& product, int quantity) {
        daily_demand[product] += quantity;
    }
};

#endif // MARKET_H
