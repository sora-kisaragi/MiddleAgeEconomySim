#include <iostream>
#include <vector>
#include "agent/person.h"
#include "market/business.h"
#include "market/market.h"
#include "system/trade_route.h"
#include "agent/government.h"
#include "agent/loan_provider.h"

void simulateDay(std::vector<Person>& people, std::vector<Business>& businesses, Market& market, 
                Government& government, LoanProvider& loan_provider, std::vector<TradeRoute>& trade_routes) {
    std::cout << "=== 1日の経済活動をシミュレート ===\n";
    
    // 生産活動
    for (auto& business : businesses) {
        int production = business.daily_production;
        business.stock += production;
        std::cout << business.product << "の生産者が" << production << "個生産しました。在庫: " << business.stock << "\n";
    }
    
    // 貿易ルートによる商品移動
    for (auto& route : trade_routes) {
        if (route.travel_time > 0) {
            std::cout << "=== 貿易ルート活動 ===\n";
            std::cout << "拠点" << route.from_location_id << " から 拠点" << route.to_location_id << " への貿易が実行されました\n";
            std::cout << "輸送品目: ";
            for (const auto& item : route.goods) {
                std::cout << item.first << "(" << item.second << "個) ";
            }
            std::cout << " (移動時間: " << route.travel_time << "日)\n";
        }
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
    
    // 政府による税収
    std::cout << "\n=== 政府活動 ===\n";
    for (auto& person : people) {
        if (person.money > 100) {  // 所持金が100以上の場合のみ課税
            bool tax_collected = government.collectTax(&person);
            if (tax_collected) {
                std::cout << person.name << "から税金を徴収しました。\n";
            }
        }
    }
    
    // 個人の消費活動
    for (auto& person : people) {
        // 収入を得る
        person.money += person.daily_income;
        std::cout << person.name << "が" << person.daily_income << "コインの収入を得ました。所持金: " << person.money << "\n";
        
        // 融資の検討（所持金が少ない場合）
        if (person.money < 50) {
            bool loan_granted = loan_provider.provideLoan(&person, 100);
            if (loan_granted) {
                std::cout << person.name << "が100コインの融資を受けました。\n";
            }
        }
        
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
    
    // 政府の政策実施
    if (government.money > 500) {
        std::cout << "\n=== 政府政策 ===\n";
        // 補助金政策の例（各業者の业种に基づいて）
        for (auto& business : businesses) {
            if (business.product == "小麦") {
                business.sector = "農業";
            } else if (business.product == "パン") {
                business.sector = "製造業";
            }
            
            // 補助金制度を設定
            government.sector_subsidies[business.sector] = 50.0f;
            bool policy_implemented = government.implementPolicy("subsidy", &business);
            if (policy_implemented) {
                std::cout << business.product << "生産者(" << business.sector << ")に補助金を支給しました。\n";
            }
        }
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
    
    std::cout << "政府の資金: " << government.money << "コイン\n";
    std::cout << "政府の支持率: " << government.approval_rating << "%\n";
    
    // 市場の日次更新
    market.clearDaily();
}

int main() {
    // 初期化
    Market market;
    market.price_volatility = 0.1f;  // 価格変動性を設定
    
    // 政府とローンプロバイダーの初期化
    Government government;
    government.money = 1000;        // 初期資金
    government.approval_rating = 75.0f;  // 初期支持率
    
    LoanProvider loan_provider;
    loan_provider.money = 5000;  // 融資可能資金
    
    // 貿易ルートの設定
    std::vector<TradeRoute> trade_routes;
    TradeRoute mainRoute;
    mainRoute.from_location_id = 1;
    mainRoute.to_location_id = 2;
    mainRoute.goods["小麦"] = 10;
    mainRoute.goods["パン"] = 5;
    mainRoute.travel_time = 3;
    trade_routes.push_back(mainRoute);
    
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
    
    std::cout << "=== 中世経済シミュレーション開始 ===\n";
    std::cout << "統合システム: 市場・政府・融資・貿易ルート\n\n";
    
    // シミュレーション実行
    for (int day = 1; day <= 5; ++day) {
        std::cout << "\n=== Day " << day << " ===\n";
        simulateDay(people, businesses, market, government, loan_provider, trade_routes);
    }
    
    // 最終結果表示
    std::cout << "\n=== シミュレーション結果 ===\n";
    for (const auto& person : people) {
        std::cout << person.name << " - 所持金: " << person.money 
                 << "コイン, 満足度: " << person.satisfaction << "\n";
    }
    std::cout << "政府最終資金: " << government.money << "コイン\n";
    std::cout << "政府最終支持率: " << government.approval_rating << "%\n";
    
    return 0;
}
