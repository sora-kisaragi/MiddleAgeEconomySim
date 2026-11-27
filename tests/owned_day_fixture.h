#pragma once
#include "system/simulation.h"

inline bool runOwnedDay(std::vector<Person>& people, std::vector<Business>& businesses,
                        Market& market, Government& government, LoanProvider& lender,
                        std::vector<TradeRoute>& routes) {
    try {
        if (!market.hasGoodsLedger()) {
            if (people.empty() || businesses.empty()) return false;
            std::vector<economy::ActorSpec> specs{
                {"government","government",economy::ActorKind::Government},
                {"lender","lender",economy::ActorKind::Lender}};
            std::vector<std::string> names{"小麦"};
            std::map<std::string,std::int64_t> sums;
            for (std::size_t i=0;i<people.size();++i) {
                if (people[i].satisfaction<0 || people[i].satisfaction>100) return false;
                specs.emplace_back("person"+std::to_string(i),"person",economy::ActorKind::Person);
                for(const auto& item:people[i].inventory) names.push_back(item);
            }
            for (std::size_t i=0;i<businesses.size();++i) {
                const auto& b=businesses[i];
                if(b.product.empty() || b.stock<0 || b.daily_production<0 || b.price<=0 ||
                   b.price>std::numeric_limits<int>::max() ||
                   b.stock>std::numeric_limits<int>::max()-b.daily_production) return false;
                sums[b.product]+=static_cast<std::int64_t>(b.stock)+b.daily_production;
                if(sums[b.product]>std::numeric_limits<int>::max()) return false;
                names.push_back(b.product);
                specs.emplace_back("business"+std::to_string(i),"business",economy::ActorKind::Business,"person0");
            }
            auto registry=std::make_shared<economy::ActorRegistry>(economy::ActorRegistry::fromSpecs(specs));
            std::sort(names.begin(),names.end()); names.erase(std::unique(names.begin(),names.end()),names.end());
            auto products=economy::assignStableIds<economy::ProductTag>(names);
            economy::GoodsSetup setup;
            for(const auto& row:products) setup.products.insert(row.second);
            for(std::size_t i=0;i<businesses.size();++i) setup.balances.emplace(
                std::make_pair(registry->actorByKey("business"+std::to_string(i)).id,products.at(businesses[i].product)),economy::Quantity(businesses[i].stock));
            for(std::size_t i=0;i<people.size();++i) for(const auto& item:people[i].inventory) {
                const auto key=std::make_pair(registry->actorByKey("person"+std::to_string(i)).id,products.at(item));
                auto it=setup.balances.find(key);
                setup.balances.insert_or_assign(key,(it==setup.balances.end()?economy::Quantity(0):it->second).add(economy::Quantity(1)));
            }
            auto goods=std::make_shared<RuntimeGoods>(registry,setup,products);
            for(std::size_t i=0;i<businesses.size();++i) {
                businesses[i].id=registry->actorByKey("business"+std::to_string(i)).id;
                businesses[i].runtime_goods=goods;
            }
            for(std::size_t i=0;i<people.size();++i) {
                people[i].id=registry->actorByKey("person"+std::to_string(i)).id;
                people[i].runtime_goods=goods; people[i].inventory.clear();
            }
            government.id=registry->actorByKey("government").id;
            lender.id=registry->actorByKey("lender").id;
            market.bindGoods(goods);
        }
        return simulateDay(people,businesses,market,government,lender,routes);
    } catch(const std::exception&) { return false; }
}
