#include "core/actor_registry.h"

#include <gtest/gtest.h>
#include <algorithm>

using namespace economy;

namespace {
std::vector<ActorSpec> villageSpecs() {
    return {{"person:p17", "商人", ActorKind::Person},
            {"business:b4", "商店", ActorKind::Business, "person:p17", true},
            {"institution:lender", "金貸", ActorKind::Lender},
            {"institution:government", "政府", ActorKind::Government}};
}
} // namespace

TEST(ActorRegistryTest, SeparatesPersonBusinessAccountAndStore) {
    const auto registry = ActorRegistry::fromSpecs(villageSpecs());
    const auto& person = registry.actorByKey("person:p17");
    const auto& business = registry.actorByKey("business:b4");
    EXPECT_EQ(registry.population(), 1U);
    EXPECT_EQ(registry.actorCount(), 4U);
    EXPECT_EQ(business.owner_person_id.value(), person.id);
    EXPECT_NE(business.id.value(), person.id.value());
    EXPECT_NE(registry.accountFor(business.id).id.value(), registry.accountFor(person.id).id.value());
    EXPECT_EQ(registry.store(registry.storeFor(business.id)).operator_business_id, business.id);
    EXPECT_EQ(registry.accountFor(business.id).owner_actor_id, business.id);
    EXPECT_EQ(registry.accountFor(person.id).owner_actor_id, person.id);
    EXPECT_THROW(registry.storeFor(person.id), std::out_of_range);
}

TEST(ActorRegistryTest, AssignmentIsIndependentOfInputOrder) {
    auto reversed = villageSpecs();
    std::reverse(reversed.begin(), reversed.end());
    const auto left = ActorRegistry::fromSpecs(villageSpecs());
    const auto right = ActorRegistry::fromSpecs(reversed);
    EXPECT_EQ(left.actorByKey("business:b4").id, right.actorByKey("business:b4").id);
    EXPECT_EQ(left.storeFor(left.actorByKey("business:b4").id),
              right.storeFor(right.actorByKey("business:b4").id));
}

TEST(ActorRegistryTest, RejectsBadReferencesAndDuplicateActors) {
    auto specs = villageSpecs();
    specs[1].owner_key = "missing";
    EXPECT_THROW(ActorRegistry::fromSpecs(specs), std::invalid_argument);
    specs[1].owner_key = "institution:lender";
    EXPECT_THROW(ActorRegistry::fromSpecs(specs), std::invalid_argument);
    specs = villageSpecs();
    specs.push_back(specs[0]);
    EXPECT_THROW(ActorRegistry::fromSpecs(specs), std::invalid_argument);
    specs = villageSpecs();
    specs[0].retail_store = true;
    EXPECT_THROW(ActorRegistry::fromSpecs(specs), std::invalid_argument);
    specs = villageSpecs();
    specs[0].kind = static_cast<ActorKind>(99);
    EXPECT_THROW(ActorRegistry::fromSpecs(specs), std::invalid_argument);
}

TEST(ActorRegistryTest, NewAndClosedBusinessesPreserveReferencesWithoutChangingPopulation) {
    auto registry = ActorRegistry::fromSpecs(villageSpecs());
    const ActorId owner = registry.actorByKey("person:p17").id;
    const ActorId second = registry.addBusiness("business:b5", "二号店", owner, true);
    const StoreId store = registry.storeFor(second);
    EXPECT_EQ(registry.population(), 1U);
    EXPECT_THROW(registry.addBusiness("business:b5", "重複", owner, false), std::invalid_argument);
    EXPECT_THROW(registry.addBusiness("business:b6", "不在", ActorId(999), false), std::invalid_argument);
    registry.closeBusiness(second);
    EXPECT_FALSE(registry.actor(second).active);
    EXPECT_FALSE(registry.store(store).active);
    EXPECT_EQ(registry.accountFor(second).owner_actor_id, second);
    EXPECT_THROW(registry.closeBusiness(owner), std::invalid_argument);
}

TEST(ActorRegistryTest, OneHundredPeopleRemainOneHundredWithBusinessesAndStores) {
    std::vector<ActorSpec> specs = {{"institution:lender", "金貸", ActorKind::Lender},
                                    {"institution:government", "政府", ActorKind::Government}};
    for (int i = 1; i <= 100; ++i) {
        specs.push_back({"person:" + std::to_string(i), "村人", ActorKind::Person});
    }
    for (int i = 1; i <= 12; ++i) {
        specs.push_back({"business:" + std::to_string(i), "事業", ActorKind::Business,
                         "person:" + std::to_string(i), i <= 2});
    }
    const auto registry = ActorRegistry::fromSpecs(specs);
    EXPECT_EQ(registry.population(), 100U);
    EXPECT_EQ(registry.actorCount(), 114U);
    EXPECT_EQ(registry.store(registry.storeFor(registry.actorByKey("business:2").id)).operator_business_id,
              registry.actorByKey("business:2").id);
}
