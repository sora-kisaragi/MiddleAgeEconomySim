#include <iostream>
#include "system/simulation.h"
#include "market/price_index.h"
#include "system/simulation_options.h"

int main(int argc, char* argv[]) {
    try {
        std::vector<std::string> arguments;
        for (int index = 1; index < argc; ++index) arguments.emplace_back(argv[index]);
        const auto options = parseSimulationOptions(arguments);
        if (options.help) {
            std::cout << "Usage: MiddleAgeEconomySim [--scenario demo|balanced|shortage|oversupply] [--days 1..10000]\n";
            return 0;
        }
        // 初期化
        Market market;
        market.setPriceVolatility(0.1f);  // 価格変動性を設定
        
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
    if (options.scenario == "demo") trade_routes.push_back(mainRoute);
    
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
    farm.price = options.scenario == "demo" ? 5 : 10;
    if (options.scenario == "balanced") farm.daily_production = 2;
    if (options.scenario == "shortage") farm.daily_production = 1;
    farm.money = 1000;
    businesses.push_back(farm);
    
    Business bakery;
    bakery.product = "パン";
    bakery.daily_production = 5;
    bakery.price = 10;
    bakery.money = 1000;
    if (options.scenario == "demo") businesses.push_back(bakery);
    
    std::cout << "=== 中世経済シミュレーション開始 ===\n";
    std::cout << "統合システム: 市場・政府・融資・貿易ルート\n\n";
    
    std::map<std::string, int> basket;
    for (const auto& business : businesses) {
        market.registerProduct(business.product, static_cast<int>(business.price));
        basket[business.product] = 1;
    }
    PriceIndex price_index(market, basket);

    auto total_money = [&]() {
        int64_t total = government.money + loan_provider.money;
        for (const auto& person : people) total += person.money;
        for (const auto& business : businesses) total += business.money;
        return total;
    };
    std::cout << "開始時の総資金: " << total_money() << "コイン\n";

    // シミュレーション実行
    for (int day = 1; day <= options.days; ++day) {
        std::cout << "\n=== Day " << day << " ===\n";
        simulateDay(people, businesses, market, government, loan_provider, trade_routes);
        const auto observation = price_index.observe(market);
        std::cout << "物価指数（基準日=100）: " << observation.value
                  << " | 前日比: " << observation.change_percent << "%\n";
    }
    
    if (options.scenario == "demo") {
        // エージェント間の直接取引の実例
        std::cout << "\n=== エージェント間直接取引 ===\n";
        if (people.size() >= 2) {
            Person& farmer = people[0];
            Person& merchant = people[1];


            std::cout << "取引前 - " << farmer.name << ": " << farmer.money << "コイン, "
                     << merchant.name << ": " << merchant.money << "コイン\n";

            // 農夫が商人から道具を購入
            bool trade_success = farmer.directTrade(&merchant, "道具", 2, 25);
            if (trade_success) {
                std::cout << "取引成功！\n";
            }

            std::cout << "取引後 - " << farmer.name << ": " << farmer.money << "コイン, "
                     << merchant.name << ": " << merchant.money << "コイン\n";
        }

        // 融資システムの実例
        std::cout << "\n=== エージェント間融資 ===\n";
        if (people.size() >= 2) {
            Person& borrower = people[0];
            Person& lender = people[1];


            std::cout << "融資前 - 借り手: " << borrower.money << "コイン, 貸し手: " << lender.money << "コイン\n";

            bool loan_success = borrower.requestLoan(&lender, 100, 5.0f);
            if (loan_success) {
                std::cout << "融資成功！\n";
            }

            std::cout << "融資後 - 借り手: " << borrower.money << "コイン, 貸し手: " << lender.money << "コイン\n";
        }

        // サービス提供の実例
        std::cout << "\n=== サービス提供 ===\n";
        if (people.size() >= 2) {
            Person& service_provider = people[1];
            Person& client = people[0];


            std::cout << "サービス提供前 - 提供者: " << service_provider.money
                     << "コイン, 顧客: " << client.money << "コイン\n";

            bool service_success = service_provider.provideService(&client, "相談サービス", 75);
            if (service_success) {
                std::cout << "サービス提供成功！\n";
            }

            std::cout << "サービス提供後 - 提供者: " << service_provider.money
                     << "コイン, 顧客: " << client.money << "コイン\n";
        }

    }

    // 最終結果表示
    std::cout << "\n=== シミュレーション結果 ===\n";
    for (const auto& person : people) {
        std::cout << person.name << " - 所持金: " << person.money 
                 << "コイン, 満足度: " << person.satisfaction << "\n";
    }
    std::cout << "政府最終資金: " << government.money << "コイン\n";
    std::cout << "政府最終支持率: " << government.approval_rating << "%\n";
    std::cout << "終了時の総資金: " << total_money() << "コイン\n";
    
    return 0;
    
    } catch (const std::exception& e) {
        std::cerr << "プログラム実行中に致命的なエラーが発生しました: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "予期しない致命的エラーが発生しました。\n";
        return 2;
    }
}
