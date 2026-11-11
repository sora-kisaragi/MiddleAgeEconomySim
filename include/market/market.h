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
    std::map<std::string, int> unassigned_stock;
    std::map<const Business*, std::map<std::string, int>> seller_listings;
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
        
        if (seller->stock < 0) return false;
        auto price_it = price.find(product);
        if (price_it == price.end() && (seller->price <= 0 || seller->price > std::numeric_limits<int>::max())) {
            return false;
        }
        const int unit_price = price_it == price.end() ? static_cast<int>(seller->price) : price_it->second;
        const int64_t total_cost = static_cast<int64_t>(unit_price) * quantity;
        if (buyer->money < total_cost || seller->money > std::numeric_limits<int64_t>::max() - total_cost) {
            return false;
        }
        if (daily_demand[product] > std::numeric_limits<int64_t>::max() - quantity) return false;
        if (price_it == price.end()) addProduct(product, unit_price);

        // Include the seller's offered stock even when this request cannot be filled.
        auto& market_stock = stock[product];
        auto& listed = seller_listings[seller][product];
        if (seller->stock > listed) {
            const int additional = seller->stock - listed;
            const int assigned = std::min(additional, unassigned_stock[product]);
            const int newly_offered = additional - assigned;
            if (market_stock > std::numeric_limits<int>::max() - newly_offered ||
                daily_supply[product] > std::numeric_limits<int64_t>::max() - newly_offered) {
                return false;
            }
            unassigned_stock[product] -= assigned;
            market_stock += newly_offered;
            daily_supply[product] += newly_offered;
            listed += additional;
        }
        if (seller->stock < quantity) {
            addDemand(product, quantity);
            return false;
        }

        if (market_stock < quantity) {
            addDemand(product, quantity);
            return false;
        }

        // 取引実行
        buyer->addMoney(-total_cost);
        seller->addMoney(total_cost);
        seller->stock -= quantity;
        listed -= quantity;
        market_stock -= quantity;

        // 取引履歴の更新
        addDemand(product, quantity);

        return true;
    }

    void registerProduct(const std::string& product, int initial_price) {
        if (initial_price <= 0) throw std::invalid_argument("Price must be positive");
        addProduct(product, initial_price);
    }

    bool sell(const std::string& product, int quantity, int price) {
        if (quantity <= 0 || price <= 0 || getStock(product) > std::numeric_limits<int>::max() - quantity) {
            return false;
        }
        if (daily_supply[product] > std::numeric_limits<int64_t>::max() - quantity) return false;
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
        if (stock[product] < quantity) {
            addDemand(product, quantity);
            throw std::invalid_argument("Insufficient stock");
        }
        const int64_t total_cost = static_cast<int64_t>(price_it->second) * quantity;
        if (total_cost > std::numeric_limits<int>::max()) {
            throw std::overflow_error("Purchase cost exceeds int range");
        }
        addDemand(product, quantity);
        stock[product] -= quantity;
        unassigned_stock[product] -= std::min(quantity, unassigned_stock[product]);
        return static_cast<int>(total_cost);
    }

    // Close the day, then carry the remaining goods into the next day's supply.
    void clearDaily() {
        for (const auto& entry : price) {
            const auto& product = entry.first;
            updatePrice(product);
            opening_price[product] = price[product];
            daily_demand[product] = 0;
            daily_supply[product] = stock[product];
            unassigned_stock[product] = stock[product];
        }
        seller_listings.clear();
    }

private:
    void addProduct(const std::string& product, int initial_price) {
        for (auto& listing : seller_listings) {
            listing.second.erase(product);
        }
        price[product] = initial_price;
        opening_price[product] = initial_price;
        stock[product] = 0;
        daily_demand[product] = 0;
        daily_supply[product] = 0;
        unassigned_stock[product] = 0;
    }

    void addStock(const std::string& product, int quantity) {
        stock[product] += quantity;
        unassigned_stock[product] += quantity;
        daily_supply[product] += quantity;
    }

    void addDemand(const std::string& product, int quantity) {
        if (daily_demand[product] > std::numeric_limits<int64_t>::max() - quantity) {
            throw std::overflow_error("Daily demand exceeds int64 range");
        }
        daily_demand[product] += quantity;
    }
};

#endif // MARKET_H
