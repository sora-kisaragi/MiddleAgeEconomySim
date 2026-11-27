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
