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
        market.stock[business.product] += business.stock;
        market.price[business.product] = business.price;
        std::cout << business.product << "が" << business.stock << "個、" << business.price << "コインで市場に出品されました。\n";
        business.stock = 0;
    }
    
    // 個人の消費活動
    for (auto& person : people) {
        // 収入を得る
        person.money += person.daily_income;
        std::cout << person.name << "が" << person.daily_income << "コインの収入を得ました。所持金: " << person.money << "\n";
        
        // 消費活動（例：食料を購入）
        const std::string food = "小麦";
        if (market.stock[food] > 0 && person.money >= market.price[food]) {
            person.money -= market.price[food];
            person.inventory.push_back(food);
            market.stock[food]--;
            std::cout << person.name << "が" << food << "を" << market.price[food] << "コインで購入しました。\n";
        }
        
        // 満足度の更新（簡易版）
        if (!person.inventory.empty()) {
            person.satisfaction = std::min(100, person.satisfaction + 5);
            std::cout << person.name << "の満足度が上がりました: " << person.satisfaction << "\n";
        } else {
            person.satisfaction = std::max(0, person.satisfaction - 10);
            std::cout << person.name << "の満足度が下がりました: " << person.satisfaction << "\n";
        }
    }
    
    std::cout << "=== 1日の経済活動が終了しました ===\n\n";
}

int main() {
    std::cout << "中世近代貨幣経済シミュレーション 開始\n\n";
    
    // エージェントの初期化
    std::vector<Person> people = {
        Person(),  // デフォルト値で初期化
    };
    people[0].id = 1;
    people[0].name = "農民A";
    people[0].money = 50;
    people[0].job = "農民";
    people[0].daily_income = 10;
    people[0].daily_expense = 5;
    
    std::vector<Person> additionalPeople = {
        Person(),  // 商人B
        Person(),  // 貴族C
    };
    additionalPeople[0].id = 2;
    additionalPeople[0].name = "商人B";
    additionalPeople[0].money = 100;
    additionalPeople[0].job = "商人";
    additionalPeople[0].daily_income = 15;
    additionalPeople[0].daily_expense = 10;
    
    additionalPeople[1].id = 3;
    additionalPeople[1].name = "貴族C";
    additionalPeople[1].money = 200;
    additionalPeople[1].job = "貴族";
    additionalPeople[1].daily_income = 20;
    additionalPeople[1].daily_expense = 15;
    additionalPeople[1].risk_tolerance = 70;  // リスク許容度高め
    
    // 人々を追加
    people.insert(people.end(), additionalPeople.begin(), additionalPeople.end());
    
    // 企業の初期化
    std::vector<Business> businesses = {
        Business(),  // 小麦生産者
        Business(),  // パン製造業者
    };
    businesses[0].id = 1;
    businesses[0].product = "小麦";
    businesses[0].price = 5;
    businesses[0].workers = 2;
    businesses[0].daily_production = 10;
    
    businesses[1].id = 2;
    businesses[1].product = "パン";
    businesses[1].price = 10;
    businesses[1].workers = 1;
    businesses[1].daily_production = 5;
    
    // 市場の初期化
    Market market;
    
    // 数日間のシミュレーション
    for (int day = 1; day <= 3; day++) {
        std::cout << "【" << day << "日目】\n";
        simulateDay(people, businesses, market);
    }
    
    // 最終状態の表示
    std::cout << "最終状態:\n";
    for (const auto& person : people) {
        std::cout << person.name << " - 所持金: " << person.money << "コイン, 満足度: " << person.satisfaction;
        std::cout << ", アイテム数: " << person.inventory.size() << "\n";
    }
    
    std::cout << "\n市場の状態:\n";
    for (const auto& [product, quantity] : market.stock) {
        std::cout << product << ": " << quantity << "個 (価格: " << market.price[product] << "コイン)\n";
    }
    
    return 0;
}
