#include "system/simulation.h"
#include <iostream>
#include <set>
#include <limits>

bool simulateDay(std::vector<Person>& people, std::vector<Business>& businesses, Market& market,
                Government& government, LoanProvider& loan_provider, std::vector<TradeRoute>& trade_routes) {
    std::cout << "=== 1日の経済活動をシミュレート ===\n";
    
    // 安全性チェック
    if (people.empty() || businesses.empty()) {
        std::cerr << "エラー: エージェントまたは企業が設定されていません。\n";
        return false;
    }
    
    try {
        // Validate every business and aggregate listing before changing any state.
        std::map<std::string, int> offered_stock;
        for (const auto& business : businesses) {
            if (business.product.empty() || business.price <= 0 ||
                business.price > std::numeric_limits<int>::max() ||
                business.daily_production < 0 || business.stock < 0 ||
                business.stock > std::numeric_limits<int>::max() - business.daily_production) {
                throw std::invalid_argument("Invalid business product, price, stock or production");
            }
            const int quantity = business.stock + business.daily_production;
            auto& total = offered_stock[business.product];
            if (total > std::numeric_limits<int>::max() - quantity) {
                throw std::overflow_error("Combined product inventory exceeds market range");
            }
            total += quantity;
        }

        // 生産活動
        for (auto& business : businesses) {
            int production = business.daily_production;
            business.stock += production;
            std::cout << business.product << "の生産者が" << production << "個生産しました。在庫: " << business.stock << "\n";
        }
        
        // 貿易ルートによる商品移動
        for (auto& route : trade_routes) {
            if (route.travel_time > 0 && !route.goods.empty()) {
                std::cout << "=== 貿易ルート活動 ===\n";
                std::cout << "拠点" << route.from_location_id << " から 拠点" << route.to_location_id << " への貿易が実行されました\n";
                std::cout << "輸送品目: ";
                for (const auto& item : route.goods) {
                    if (item.second > 0) {
                        std::cout << item.first << "(" << item.second << "個) ";
                    }
                }
                std::cout << " (移動時間: " << route.travel_time << "日)\n";
            }
        }
    
    // Rebuild each product's listings from the businesses' authoritative inventory.
    std::set<std::string> initialized_products;
    for (auto& business : businesses) {
        if (initialized_products.insert(business.product).second) {
            const int quoted_price = market.getPrice(business.product);
            market.registerProduct(business.product, quoted_price > 0 ? quoted_price : static_cast<int>(business.price));
        }
        if (business.stock > 0 && !market.sell(business.product, business.stock, static_cast<int>(business.price))) {
            throw std::overflow_error("Cannot list business inventory");
        }
        std::cout << business.product << "が" << business.stock << "個、"
                  << market.getPrice(business.product) << "コインで市場に出品されました。\n";
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
    for (size_t person_index = 0; person_index < people.size(); ++person_index) {
        auto& person = people[person_index];
        auto& employer = businesses[person_index % businesses.size()];
        // Phase 1 assigns employers by position; wages are transfers from their cash.
        if (person.daily_income > 0 && person.provideService(&employer, "労働", person.daily_income)) {
            std::cout << person.name << "が" << person.daily_income << "コインの収入を得ました。所持金: " << person.money << "\n";
        }
        
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
            if (market.getPrice(food) > 0 && person.money >= market.getPrice(food)) {
                auto seller = std::find_if(businesses.begin(), businesses.end(), [&](const Business& business) {
                    return business.product == food && business.stock > 0;
                });
                if (seller == businesses.end()) {
                    seller = std::find_if(businesses.begin(), businesses.end(), [&](const Business& business) {
                        return business.product == food;
                    });
                }
                const int cost = market.getPrice(food);
                if (seller != businesses.end() && market.transact(&person, &*seller, food, 1)) {
                    person.inventory.push_back(food);
                    std::cout << person.name << "が" << food << "を" << cost << "コインで購入しました。\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "商品の購入中にエラーが発生: " << e.what() << "\n";
        }
        
        // 満足度の更新（簡易版）
        if (person.removeInventoryItem(food)) {
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
    
    // 日末に需給を集計し、翌日の価格を決定
    market.clearDaily();

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
    
    
    return true;
    } catch (const std::exception& e) {
        std::cerr << "シミュレーション中に重大なエラーが発生しました: " << e.what() << "\n";
        std::cerr << "シミュレーションを停止します。\n";
        return false;
    } catch (...) {
        std::cerr << "予期しないエラーが発生しました。シミュレーションを停止します。\n";
        return false;
    }
}

