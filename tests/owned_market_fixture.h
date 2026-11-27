#pragma once
#include "market/market.h"

// Test convenience operations always execute real production, owner-backed
// transfers and cash payments. No anonymous inventory or mock balances.
class OwnedMarket : public Market {
public:
    OwnedMarket() {
        std::vector<economy::ActorSpec> specs{{"buyer", "buyer", economy::ActorKind::Person},
            {"government", "government", economy::ActorKind::Government},
            {"lender", "lender", economy::ActorKind::Lender}};
        for (int i = 0; i < 64; ++i) specs.emplace_back("seller" + std::to_string(i), "seller", economy::ActorKind::Business, "buyer");
        registry = std::make_shared<economy::ActorRegistry>(economy::ActorRegistry::fromSpecs(specs));
        economy::GoodsSetup setup;
        for (const auto& name : {"grain", "bread", "tools", "小麦", "パン", "道具", "new_product"}) {
            products.emplace(name, economy::ProductId(products.size()+1));
            setup.products.insert(products.at(name));
        }
        goods = std::make_shared<RuntimeGoods>(registry, setup, products);
        bindGoods(goods); goods->beginDay();
        buyer.id = registry->actorByKey("buyer").id; buyer.runtime_goods = goods;
        buyer.money = std::numeric_limits<std::int64_t>::max()/2;
    }
    void clearDaily() { Market::clearDaily(); goods->beginDay(); }
    bool sell(const std::string& product, int quantity, int quote) {
        if (product.empty() || !products.count(product) || quantity <= 0 || quote <= 0 ||
            getStock(product) > std::numeric_limits<int>::max()-quantity) return false;
        if (!getPrice(product)) registerProduct(product, quote);
        auto& seller = suppliers[product];
        if (!seller.id) {
            seller.id = registry->actorByKey("seller" + std::to_string(next++)).id;
            seller.product = product; seller.price = quote; seller.runtime_goods = goods;
        }
        if (!goods->move(*seller.id, product, quantity, economy::GoodsFlowKind::Production)) return false;
        listOwner(seller); noteAdditionalSupply(product, quantity); return true;
    }
    int buy(const std::string& product, int quantity) {
        if (quantity <= 0 || !getPrice(product)) throw std::invalid_argument("invalid purchase");
        const auto cost = static_cast<std::int64_t>(quantity)*getPrice(product);
        if (quantity <= getStock(product) && cost > std::numeric_limits<int>::max()) throw std::overflow_error("legacy result range");
        auto& seller = suppliers[product];
        if (!seller.id) {
            seller.id=registry->actorByKey("seller"+std::to_string(next++)).id;
            seller.product=product; seller.price=getPrice(product); seller.runtime_goods=goods;
            listOwner(seller);
        }
        Business* source = &seller;
        for (auto* owner : external) if(owner->product==product && owner->goodsStock()>=quantity) { source=owner; break; }
        if (!Market::transact(&buyer, source, product, quantity)) throw std::invalid_argument("unfilled purchase");
        return static_cast<int>(cost);
    }
    bool transact(Person* person, Business* business, const std::string& product, int quantity) {
        if (!person || !business || product.empty() || business->product!=product || quantity<=0 ||
            (!getPrice(product) && (business->price<=0 || business->price>std::numeric_limits<int>::max()))) return false;
        const auto quote=getPrice(product)>0?getPrice(product):business->price;
        const auto cost=static_cast<std::int64_t>(quantity)*quote;
        if(person->money<cost || business->money<0 || business->money>std::numeric_limits<std::int64_t>::max()-cost) return false;
        if (!getPrice(product)) registerProduct(product, business->price);
        if (!business->runtime_goods) {
            if (business->stock < 0) return false;
            const auto pool=suppliers.find(product);
            const auto pooled=pool==suppliers.end()?0:pool->second.goodsStock();
            const auto extra=std::max<std::int64_t>(0,business->stock-pooled);
            if(extra>std::numeric_limits<int>::max()-getStock(product)) return false;
            business->id = registry->actorByKey("seller" + std::to_string(next++)).id;
            business->runtime_goods = goods;
            if (!products.count(business->product)) return false;
            if (business->stock) {
                auto it = suppliers.find(business->product);
                const auto available = it == suppliers.end() ? 0 : it->second.goodsStock();
                const auto fromSupply = std::min<std::int64_t>(available, business->stock);
                if (fromSupply) goods->move(*it->second.id, business->product, fromSupply, economy::GoodsFlowKind::Transfer, business->id);
                if (business->stock > fromSupply) {
                    goods->move(*business->id, business->product, business->stock-fromSupply, economy::GoodsFlowKind::Production);
                    noteAdditionalSupply(product,business->stock-fromSupply);
                }
            }
        }
        if (!person->runtime_goods) { person->id = buyer.id; person->runtime_goods = goods; }
        listOwner(*business);
        if(std::find(external.begin(),external.end(),business)==external.end()) external.push_back(business);
        return Market::transact(person, business, product, quantity);
    }
    std::shared_ptr<economy::ActorRegistry> registry;
    std::shared_ptr<RuntimeGoods> goods;
private:
    std::map<std::string,economy::ProductId> products;
    std::map<std::string,Business> suppliers;
    Person buyer;
    std::vector<Business*> external;
    int next=0;
};
