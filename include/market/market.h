#ifndef MARKET_H
#define MARKET_H

#include <string>
#include <map>
#include <vector>
#include "../agent/person.h"
#include "../market/business.h"

class Market {
public:
    std::map<std::string, int> stock;
    std::map<std::string, int> price;
    std::map<std::string, std::vector<int>> demand_history;
    std::map<std::string, std::vector<int>> supply_history;
    float price_volatility;

    Market() : price_volatility(0.1f) {}

    void addProduct(const std::string& product, int initial_price) {
        price[product] = initial_price;
        stock[product] = 0;
        if (demand_history.find(product) == demand_history.end()) {
            demand_history[product] = std::vector<int>();
            demand_history[product].push_back(0);  // 初期需要を0として記録
        }
        if (supply_history.find(product) == supply_history.end()) {
            supply_history[product] = std::vector<int>();
            supply_history[product].push_back(0);  // 初期供給を0として記録
        }
    }

    void addStock(const std::string& product, int quantity) {
        if (stock.find(product) == stock.end()) {
            stock[product] = 0;
        }
        stock[product] += quantity;
        if (supply_history.find(product) == supply_history.end()) {
            supply_history[product] = std::vector<int>();
        }
        supply_history[product].push_back(quantity);
        
        // 需要と供給の不均衡をチェック
        if (!demand_history[product].empty()) {
            int latest_demand = demand_history[product].back();
            if (quantity < latest_demand) {
                price_volatility *= 1.1f;  // 供給不足で価格変動性を増加
            }
        }
    }

    void addDemand(const std::string& product, int quantity) {
        if (demand_history.find(product) == demand_history.end()) {
            demand_history[product] = std::vector<int>();
        }
        demand_history[product].push_back(quantity);

        // 需要が供給を上回る場合、価格変動性を増加
        if (!supply_history[product].empty()) {
            int latest_supply = supply_history[product].back();
            if (quantity > latest_supply) {
                price_volatility *= 1.1f;
            }
        }
    }

    int getPrice(const std::string& product) const {
        auto it = price.find(product);
        return it != price.end() ? it->second : 0;
    }

    int getStock(const std::string& product) const {
        auto it = stock.find(product);
        return it != stock.end() ? it->second : 0;
    }

    void updatePrice(const std::string& product) {
        auto& supply_hist = supply_history[product];
        auto& demand_hist = demand_history[product];
        
        if (supply_hist.empty() || demand_hist.empty()) {
            return;
        }

        int current_supply = supply_hist.back();
        int current_demand = demand_hist.back();
        
        if (current_supply == 0) return;

        float demand_supply_ratio = static_cast<float>(current_demand) / current_supply;
        float price_change = (demand_supply_ratio - 1.0f) * price_volatility;
        
        auto& current_price = price[product];
        current_price = static_cast<int>(current_price * (1.0f + price_change));
        if (current_price < 1) current_price = 1;
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
        if (buyer->money < total_cost || seller->stock < quantity) {
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
        
        // 価格更新
        updatePrice(product);

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
        auto price_it = price.find(product);
        if (price_it == price.end()) {
            throw std::invalid_argument("Product not found in market");
        }
        if (stock[product] < quantity) {
            throw std::invalid_argument("Insufficient stock");
        }
        int total_cost = price_it->second * quantity;
        stock[product] -= quantity;
        addDemand(product, quantity);
        updatePrice(product);
        return total_cost;
    }

    void clearDaily() {
        // 日次の統計情報をリセット
        for (auto& [product, history] : supply_history) {
            if (!history.empty()) {
                history.clear();
                history.push_back(0);
            }
        }
        for (auto& [product, history] : demand_history) {
            if (!history.empty()) {
                history.clear();
                history.push_back(0);
            }
        }
    }
};

#endif // MARKET_H
