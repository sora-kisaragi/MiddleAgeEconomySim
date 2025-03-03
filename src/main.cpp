#include <iostream>
#include <vector>
#include "agent/person.h"
#include "market/business.h"
#include "market/market.h"
#include "system/trade_route.h"

void simulateDay(std::vector<Person>& people, std::vector<Business>& businesses, Market& market) {
    std::cout << "=== 1日の経済活動をシミュレート ===\n";
    
    // 生産活動
    for (auto& business : businesses) {
        int production = business.daily_production;
        business.stock += production;
        std::cout << business.product << "の生産者が" << production << "個生産しました。在庫: " << business.stock << "\n";
    }
    
    // 市場への出品
    for (auto& business : businesses) {
        try {
            // 商品が登録されていなければ登録
            try {
                market.getPrice(business.product);
            } catch (const std::invalid_argument&) {
                market.registerProduct(business.product, business.price);
            }
            
            // 在庫を市場に追加
            market.sell(business.product, business.stock, business.price);
            std::cout << business.product << "が" << business.stock << "個、" 
                     << market.getPrice(business.product) << "コインで市場に出品されました。\n";
            business.stock = 0;
        } catch (const std::exception& e) {
            std::cerr << "商品の出品中にエラーが発生: " << e.what() << "\n";
        }
    }
    
    // 個人の消費活動
    for (auto& person : people) {
        // 収入を得る
        person.money += person.daily_income;
        std::cout << person.name << "が" << person.daily_income << "コインの収入を得ました。所持金: " << person.money << "\n";
        
        // 消費活動（例：食料を購入）
        const std::string food = "小麦";
        try {
            if (market.getStock(food) > 0 && person.money >= market.getPrice(food)) {
                int64_t cost = market.buy(food, 1);
                person.money -= cost;
                person.inventory.push_back(food);
                std::cout << person.name << "が" << food << "を" << cost << "コインで購入しました。\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "商品の購入中にエラーが発生: " << e.what() << "\n";
        }
        
        // 満足度の更新（簡易版）
        if (!person.inventory.empty()) {
            int satisfaction_increase = 10;
            person.setSatisfaction(std::min(100, person.satisfaction + satisfaction_increase));
            std::cout << person.name << "の満足度が" << satisfaction_increase << "ポイント上昇し、" 
                     << person.satisfaction << "になりました。\n";
        } else {
            int satisfaction_decrease = 5;
            person.setSatisfaction(std::max(0, person.satisfaction - satisfaction_decrease));
            std::cout << person.name << "の満足度が" << satisfaction_decrease << "ポイント低下し、" 
                     << person.satisfaction << "になりました。\n";
        }
        person.inventory.clear();
    }
    
    // 市場の状況を表示
    std::cout << "\n=== 市場の状況 ===\n";
    for (const auto& product : {"小麦", "パン", "道具"}) {
        try {
            std::cout << product << ": " << market.getStock(product) << "個 (価格: " 
                     << market.getPrice(product) << "コイン)\n";
        } catch (const std::invalid_argument&) {
            // 商品が存在しない場合はスキップ
            continue;
        }
    }
    
    // 市場の日次更新
    market.clearDaily();
}

int main() {
    // 初期化
    Market market;
    market.setPriceVolatility(0.1f);  // 価格変動性を設定
    
    std::vector<Person> people;
    Person farmer;
    farmer.name = "農夫";
    farmer.job = "農業";
    farmer.setDailyIncome(50);
    farmer.setDailyExpense(30);
    people.push_back(farmer);
    
    Person merchant;
    merchant.name = "商人";
    merchant.job = "商売";
    merchant.setDailyIncome(80);
    merchant.setDailyExpense(40);
    people.push_back(merchant);
    
    std::vector<Business> businesses;
    Business farm;
    farm.product = "小麦";
    farm.daily_production = 10;
    farm.price = 5;
    businesses.push_back(farm);
    
    Business bakery;
    bakery.product = "パン";
    bakery.daily_production = 5;
    bakery.price = 10;
    businesses.push_back(bakery);
    
    // シミュレーション実行
    for (int day = 1; day <= 5; ++day) {
        std::cout << "\n=== Day " << day << " ===\n";
        simulateDay(people, businesses, market);
    }
    
    return 0;
}
