#include "system/simulation.h"
#include <gtest/gtest.h>

TEST(RuntimeGoodsTest, ActualDailyPathOwnsGoodsAndListingsAreOnlyReferences) {
    auto registry = economy::ActorRegistry::fromSpecs({
        {"person", "person", economy::ActorKind::Person},
        {"farm", "farm", economy::ActorKind::Business, "person"},
        {"government", "government", economy::ActorKind::Government},
        {"lender", "lender", economy::ActorKind::Lender}});
    const auto person = registry.actorByKey("person").id;
    const auto farm = registry.actorByKey("farm").id;
    const economy::ProductId wheat(1);
    economy::GoodsSetup setup;
    setup.products.insert(wheat);
    setup.balances.emplace(std::make_pair(farm, wheat), economy::Quantity(5));
    auto goods = std::make_shared<RuntimeGoods>(registry, setup,
        std::map<std::string, economy::ProductId>{{"小麦", wheat}});
    std::vector<Person> people(1);
    people[0].id = person; people[0].money = 100; people[0].runtime_goods = goods;
    std::vector<Business> businesses(1);
    auto& business = businesses[0];
    business.id = farm; business.product = "小麦"; business.price = 5;
    business.daily_production = 2; business.runtime_goods = goods;
    // Stale initialization fields cannot become a second balance after binding.
    business.stock = 999;
    people[0].inventory = {"小麦", "小麦"};
    Market market; market.bindGoods(goods);
    market.registerProduct("小麦", 5);
    market.listOwner(business); market.listOwner(business);
    { Business copy = business; market.listOwner(copy); }
    EXPECT_EQ(market.getStock("小麦"), 5);
    EXPECT_EQ(business.goodsStock(), 5);
    Government government; LoanProvider lender; std::vector<TradeRoute> routes;
    for (int day = 0; day < 3; ++day) {
        ASSERT_TRUE(simulateDay(people, businesses, market, government, lender, routes));
        EXPECT_TRUE(goods->audit());
        EXPECT_EQ(goods->balance(person, "小麦"), 0);
        EXPECT_EQ(market.getStock("小麦"), 6 + day);
        EXPECT_EQ(business.goodsStock(), 6 + day);
        EXPECT_EQ(people[0].money + business.money, 100);
    }
    std::map<economy::ActorId, std::int64_t> reconstructed{{farm,5}, {person,0}};
    const auto history = goods->history();
    ASSERT_EQ(history.size(), 12U);
    for (const auto& entry : history) {
        const bool positive = entry.kind == economy::GoodsEntryKind::Production ||
                              entry.kind == economy::GoodsEntryKind::Receipt;
        reconstructed[entry.actor_id] += (positive ? 1 : -1) * entry.quantity.value();
    }
    EXPECT_EQ(reconstructed[farm], business.goodsStock());
    EXPECT_EQ(reconstructed[person], goods->balance(person, "小麦"));
    EXPECT_FALSE(market.sell("小麦", 100, 5));
    EXPECT_THROW(market.buy("小麦", 1), std::logic_error);
    EXPECT_THROW(business.setStock(20), std::logic_error);
    EXPECT_FALSE(people[0].directTrade(&business, "小麦", 1, 5));
}

TEST(RuntimeGoodsTest, ProcessingAndFailedPurchasePreserveOwnerBalancesAndCash) {
    auto registry = economy::ActorRegistry::fromSpecs({
        {"owner", "owner", economy::ActorKind::Person},
        {"mill", "mill", economy::ActorKind::Business, "owner"},
        {"government", "government", economy::ActorKind::Government},
        {"lender", "lender", economy::ActorKind::Lender}});
    const auto owner = registry.actorByKey("owner").id;
    const auto mill = registry.actorByKey("mill").id;
    const economy::ProductId wheat(1), bread(2);
    economy::GoodsSetup setup;
    setup.products = {wheat, bread};
    setup.balances.emplace(std::make_pair(mill, wheat), economy::Quantity(4));
    setup.labor.emplace(mill, economy::Quantity(3));
    setup.recipes.emplace("bread", economy::ProcessingRecipe{{{wheat, economy::Quantity(2)}},
        {{bread, economy::Quantity(3)}}, economy::Quantity(1)});
    auto goods = std::make_shared<RuntimeGoods>(registry, setup,
        std::map<std::string, economy::ProductId>{{"小麦",wheat},{"パン",bread}});
    goods->beginDay();
    ASSERT_TRUE(goods->process(mill, "bread", 1));
    EXPECT_FALSE(goods->process(mill, "bread", 2));
    EXPECT_EQ(goods->balance(mill, "小麦"), 2);
    EXPECT_EQ(goods->balance(mill, "パン"), 3);
    EXPECT_EQ(goods->history().size(), 2U);
    EXPECT_TRUE(goods->audit());
    Person buyer; buyer.id = owner; buyer.runtime_goods = goods; buyer.money = 9;
    Business seller; seller.id = mill; seller.runtime_goods = goods; seller.product = "パン";
    Market market; market.bindGoods(goods); market.registerProduct("パン", 10); market.listOwner(seller);
    EXPECT_FALSE(market.transact(&buyer, &seller, "パン", 1));
    EXPECT_EQ(buyer.money, 9); EXPECT_EQ(seller.money, 0);
    EXPECT_EQ(goods->balance(owner, "パン"), 0);
    EXPECT_EQ(goods->balance(mill, "パン"), 3);
    buyer.money = 100;
    ASSERT_TRUE(market.transact(&buyer, &seller, "パン", 1));
    EXPECT_EQ(buyer.money, 90); EXPECT_EQ(seller.money, 10);
    ASSERT_TRUE(buyer.removeInventoryItem("パン"));
    ASSERT_TRUE(goods->move(mill, "パン", 1, economy::GoodsFlowKind::Disposal));
    EXPECT_EQ(market.getStock("パン"), 1);
    EXPECT_TRUE(goods->audit());
    seller.workers = 1; seller.daily_production = 1; seller.stock = std::numeric_limits<int>::max();
    EXPECT_NO_THROW(seller.produce());
    EXPECT_EQ(seller.goodsStock(), 2);
    EXPECT_TRUE(goods->audit());
    Business fake = seller; fake.id = owner;
    EXPECT_THROW(market.listOwner(fake), std::invalid_argument);
}

TEST(RuntimeGoodsTest, RetiredUnboundExecutionCannotMutateGoodsOrCash) {
    EXPECT_THROW((RuntimeGoods(std::shared_ptr<const economy::ActorRegistry>{},
        economy::GoodsSetup{}, std::map<std::string,economy::ProductId>{})), std::invalid_argument);
    Market market;
    Person person; person.money=100; person.inventory={"小麦"};
    Business business; business.product="小麦"; business.price=5;
    business.stock=10; business.workers=1; business.daily_production=2;
    EXPECT_FALSE(market.sell("小麦",10,5));
    EXPECT_THROW(market.buy("小麦",1),std::logic_error);
    EXPECT_FALSE(market.transact(&person,&business,"小麦",1));
    EXPECT_THROW(business.produce(),std::logic_error);
    EXPECT_THROW(business.sell(1),std::logic_error);
    EXPECT_FALSE(person.directTrade(&business,"小麦",1,5));
    std::vector<Person> people{person}; std::vector<Business> businesses{business};
    Government government; LoanProvider lender; std::vector<TradeRoute> routes;
    EXPECT_FALSE(simulateDay(people,businesses,market,government,lender,routes));
    EXPECT_EQ(people[0].money,100); EXPECT_EQ(businesses[0].money,0);
    EXPECT_EQ(businesses[0].stock,10);
    EXPECT_EQ(people[0].inventory,(std::vector<std::string>{"小麦"}));
    EXPECT_EQ(market.getStock("小麦"),0);
}

TEST(RuntimeGoodsTest, RecipientOverflowRollsBackMarketCashAndBothOwners) {
    auto registry=std::make_shared<economy::ActorRegistry>(economy::ActorRegistry::fromSpecs({
        {"buyer","buyer",economy::ActorKind::Person},
        {"seller","seller",economy::ActorKind::Business,"buyer"},
        {"government","government",economy::ActorKind::Government},
        {"lender","lender",economy::ActorKind::Lender}}));
    const auto buyerId=registry->actorByKey("buyer").id;
    const auto sellerId=registry->actorByKey("seller").id;
    economy::ProductId wheat(1); economy::GoodsSetup setup;
    setup.products={wheat};
    setup.balances.emplace(std::make_pair(buyerId,wheat),economy::Quantity(std::numeric_limits<std::int64_t>::max()));
    setup.balances.emplace(std::make_pair(sellerId,wheat),economy::Quantity(5));
    auto goods=std::make_shared<RuntimeGoods>(registry,setup,
        std::map<std::string,economy::ProductId>{{"小麦",wheat}});
    goods->beginDay();
    Person buyer; buyer.id=buyerId; buyer.runtime_goods=goods; buyer.money=100;
    Business seller; seller.id=sellerId; seller.product="小麦"; seller.runtime_goods=goods;
    Market market; market.bindGoods(goods); market.registerProduct("小麦",10); market.listOwner(seller);
    ASSERT_FALSE(market.transact(&buyer,&seller,"小麦",1));
    EXPECT_EQ(buyer.money,100); EXPECT_EQ(seller.money,0);
    EXPECT_EQ(seller.goodsStock(),5); EXPECT_EQ(market.getStock("小麦"),5);
    EXPECT_EQ(goods->balance(buyerId,"小麦"),std::numeric_limits<std::int64_t>::max());
    EXPECT_TRUE(goods->history().empty()); EXPECT_TRUE(goods->audit());
}

TEST(RuntimeGoodsTest, InvalidNameBindingsCannotAliasOwnedProducts) {
    auto registry=std::make_shared<economy::ActorRegistry>(economy::ActorRegistry::fromSpecs({
        {"buyer","buyer",economy::ActorKind::Person},
        {"seller","seller",economy::ActorKind::Business,"buyer"},
        {"government","government",economy::ActorKind::Government},
        {"lender","lender",economy::ActorKind::Lender}}));
    economy::GoodsSetup setup;
    const economy::ProductId grain(1),bread(2);
    setup.products={grain,bread};
    setup.balances.emplace(std::make_pair(registry->actorByKey("seller").id,grain),economy::Quantity(5));
    for(const auto& names:std::vector<std::map<std::string,economy::ProductId>>{
        {{"grain",grain},{"bread",grain}}, {{"",grain},{"bread",bread}},
        {{"grain",grain},{"bread",economy::ProductId(3)}}, {{"grain",grain}}}) {
        EXPECT_THROW((RuntimeGoods(registry,setup,names)),std::invalid_argument);
        EXPECT_THROW((RuntimeGoods(*registry,setup,names)),std::invalid_argument);
    }
    EXPECT_NO_THROW((RuntimeGoods(registry,setup,{{"grain",grain},{"bread",bread}})));
}

TEST(RuntimeGoodsTest, BoundMarketValidatesInvalidInputsWithoutFixturePrechecks) {
    auto registry=std::make_shared<economy::ActorRegistry>(economy::ActorRegistry::fromSpecs({
        {"buyer","buyer",economy::ActorKind::Person},
        {"seller","seller",economy::ActorKind::Business,"buyer"},
        {"government","government",economy::ActorKind::Government},
        {"lender","lender",economy::ActorKind::Lender}}));
    const auto buyerId=registry->actorByKey("buyer").id;
    const auto sellerId=registry->actorByKey("seller").id;
    const economy::ProductId grain(1);
    economy::GoodsSetup setup;
    setup.products={grain}; setup.balances.emplace(std::make_pair(sellerId,grain),economy::Quantity(5));
    auto goods=std::make_shared<RuntimeGoods>(registry,setup,
        std::map<std::string,economy::ProductId>{{"grain",grain}});
    goods->beginDay();
    Person buyer; buyer.id=buyerId; buyer.runtime_goods=goods; buyer.money=100;
    Business seller; seller.id=sellerId; seller.runtime_goods=goods; seller.product="grain";
    Market market; market.bindGoods(goods); market.registerProduct("grain",10); market.listOwner(seller);
    const auto unchanged = [&]() {
        EXPECT_EQ(goods->balance(sellerId,"grain"),5);
        EXPECT_EQ(goods->balance(buyerId,"grain"),0);
        EXPECT_EQ(seller.money,0); EXPECT_EQ(buyer.money,100);
        EXPECT_TRUE(goods->history().empty()); EXPECT_TRUE(goods->audit());
    };
    for(int quantity:{0,-1}) {
        EXPECT_FALSE(market.transact(&buyer,&seller,"grain",quantity)); unchanged();
    }
    EXPECT_FALSE(market.transact(nullptr,&seller,"grain",1)); unchanged();
    EXPECT_FALSE(market.transact(&buyer,nullptr,"grain",1)); unchanged();
    EXPECT_FALSE(market.transact(&buyer,&seller,"bread",1)); unchanged();
    EXPECT_FALSE(market.transact(&buyer,&seller,"",1)); unchanged();
    buyer.money=9;
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",1)); EXPECT_EQ(buyer.money,9);
    buyer.money=-1;
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",1)); EXPECT_EQ(buyer.money,-1);
    buyer.money=100; seller.money=-1;
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",1)); EXPECT_EQ(seller.money,-1);
    seller.money=std::numeric_limits<std::int64_t>::max();
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",1));
    EXPECT_EQ(seller.money,std::numeric_limits<std::int64_t>::max());
    seller.money=0; unchanged();
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",6)); unchanged();
    buyer.id.reset();
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",1)); buyer.id=buyerId; unchanged();
    buyer.runtime_goods.reset();
    EXPECT_FALSE(market.transact(&buyer,&seller,"grain",1)); buyer.runtime_goods=goods; unchanged();
    Market unlisted; unlisted.bindGoods(goods); unlisted.registerProduct("grain",10);
    EXPECT_FALSE(unlisted.transact(&buyer,&seller,"grain",1)); unchanged();
    ASSERT_TRUE(market.transact(&buyer,&seller,"grain",1));
    EXPECT_EQ(buyer.money,90); EXPECT_EQ(seller.money,10);
    EXPECT_EQ(goods->balance(buyerId,"grain"),1); EXPECT_EQ(goods->balance(sellerId,"grain"),4);
    EXPECT_TRUE(goods->audit());
}
