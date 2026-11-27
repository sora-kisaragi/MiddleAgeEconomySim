#include "system/simulation.h"
#include <gtest/gtest.h>

using namespace economy;

class RegistryRuntimeTest : public ::testing::Test {
protected:
    ActorRegistry registry = ActorRegistry::fromSpecs({
        {"person:p17", "P17", ActorKind::Person},
        {"business:b4", "B4", ActorKind::Business, "person:p17", true},
        {"government", "government", ActorKind::Government},
        {"lender", "lender", ActorKind::Lender}});
    ActorId owner = registry.actorByKey("person:p17").id;
    ActorId shop = registry.actorByKey("business:b4").id;
    ProductId wheat{1};
    std::shared_ptr<RuntimeGoods> goods;
    std::vector<Person> people{1};
    std::vector<Business> businesses{1};
    Market market;
    Government government;
    LoanProvider lender;
    std::vector<TradeRoute> routes;

    void SetUp() override {
        GoodsSetup setup;
        setup.products.insert(wheat);
        setup.balances.emplace(std::make_pair(owner, wheat), Quantity(2));
        setup.balances.emplace(std::make_pair(shop, wheat), Quantity(5));
        goods = std::make_shared<RuntimeGoods>(registry, setup,
            std::map<std::string, ProductId>{{"小麦", wheat}});
        people[0].id = owner; people[0].money = 100;
        people[0].daily_income = 0; people[0].runtime_goods = goods;
        businesses[0].id = shop; businesses[0].money = 200;
        businesses[0].product = "小麦"; businesses[0].price = 5;
        businesses[0].daily_production = 0; businesses[0].runtime_goods = goods;
        government.id = registry.actorByKey("government").id;
        lender.id = registry.actorByKey("lender").id;
        market.bindGoods(goods); market.registerProduct("小麦", 5);
        market.listOwner(businesses[0]);
    }
    bool run() { return simulateDay(people, businesses, market, government, lender, routes); }
    void expectUnchangedRejection() {
        const auto cash = people[0].money;
        const auto business_cash = businesses[0].money;
        const auto satisfaction = people[0].satisfaction;
        const auto displayed = market.getStock("小麦");
        EXPECT_FALSE(run());
        EXPECT_EQ(people[0].money, cash);
        EXPECT_EQ(businesses[0].money, business_cash);
        EXPECT_EQ(government.money, 0); EXPECT_EQ(lender.money, 0);
        EXPECT_EQ(people[0].satisfaction, satisfaction);
        EXPECT_EQ(goods->balance(owner, "小麦"), 2);
        EXPECT_EQ(goods->balance(shop, "小麦"), 5);
        EXPECT_EQ(market.getStock("小麦"), displayed);
        EXPECT_TRUE(goods->history().empty());
    }
};

TEST_F(RegistryRuntimeTest, P17B4StoreKeepsSeparateCashAndGoodsThroughActualDay) {
    const auto store = registry.storeFor(shop);
    EXPECT_EQ(registry.store(store).operator_business_id, shop);
    EXPECT_EQ(registry.accountFor(owner).owner_actor_id, owner);
    EXPECT_EQ(registry.accountFor(shop).owner_actor_id, shop);
    EXPECT_NE(registry.accountFor(owner).id, registry.accountFor(shop).id);
    ASSERT_TRUE(run());
    EXPECT_EQ(people[0].money, 95); EXPECT_EQ(businesses[0].money, 205);
    // Household receives one then consumes one. The store only displays B4 stock.
    EXPECT_EQ(goods->balance(owner, "小麦"), 2);
    EXPECT_EQ(goods->balance(shop, "小麦"), 4);
    EXPECT_EQ(market.getStock("小麦"), 4);
    EXPECT_TRUE(goods->audit());
    EXPECT_EQ(registry.population(), 1U);
    const auto history = goods->history(); ASSERT_EQ(history.size(), 3U);
    registry.closeBusiness(shop);
    const auto added = registry.addBusiness("business:b5", "B5", owner, true);
    EXPECT_EQ(registry.population(), 1U);
    EXPECT_NE(added, shop);
    for (const auto& entry : history) EXPECT_NO_THROW(registry.actor(entry.actor_id));
    EXPECT_FALSE(registry.actor(shop).active);
    EXPECT_FALSE(registry.store(store).active);
    EXPECT_EQ(registry.accountFor(shop).owner_actor_id, shop);
    EXPECT_EQ(goods->balance(shop, "小麦"), 4);
}

TEST_F(RegistryRuntimeTest, DuplicateHouseholdRejectedBeforeAnyMutation) {
    people.push_back(people[0]); expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, DuplicateBusinessRejectedBeforeAnyMutation) {
    businesses.push_back(businesses[0]); expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, WrongHouseholdKindRejectedBeforeAnyMutation) {
    people[0].id = shop; expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, MissingHouseholdReferenceRejectedBeforeAnyMutation) {
    people[0].id = ActorId(999); expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, WrongBusinessKindRejectedBeforeAnyMutation) {
    businesses[0].id = owner; expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, ClosedBusinessRejectedBeforeAnyMutation) {
    registry.closeBusiness(shop); expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, WrongInstitutionKindRejectedBeforeAnyMutation) {
    government.id = owner; expectUnchangedRejection();
}
TEST_F(RegistryRuntimeTest, MissingInstitutionRejectedBeforeAnyMutation) {
    lender.id.reset(); expectUnchangedRejection();
}
