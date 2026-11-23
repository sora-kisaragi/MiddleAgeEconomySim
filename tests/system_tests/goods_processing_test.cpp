#include "core/settlement_service.h"
#include <gtest/gtest.h>
#include <limits>

using namespace economy;

namespace {
class GoodsProcessingTest : public ::testing::Test {
protected:
    ActorRegistry registry = ActorRegistry::fromSpecs({
        {"owner", "owner", ActorKind::Person},
        {"bakery", "bakery", ActorKind::Business, "owner"},
        {"lender", "lender", ActorKind::Lender},
        {"government", "government", ActorKind::Government}});
    ActorId owner = registry.actorByKey("owner").id;
    ActorId bakery = registry.actorByKey("bakery").id;
    ProductId wheat{1}, fuel{2}, bread{3};

    GoodsSetup setup() const {
        return {{wheat, fuel, bread},
            {{"bread", {{{wheat, Quantity(2)}, {fuel, Quantity(1)}},
                         {{bread, Quantity(3)}}, Quantity(2)}}},
            {{{bakery, wheat}, Quantity(10)}, {{bakery, fuel}, Quantity(5)},
             {{owner, wheat}, Quantity(100)}}, {{bakery, Quantity(10)}}};
    }
    GoodsProcessingCommand command(std::uint64_t id = 1, std::uint64_t version = 0,
                                   std::int64_t batches = 2) const {
        return {CommandId(id), std::string(64, 'a'), version,
                GameTime(1, 6, id), bakery, owner, "bread", Quantity(batches)};
    }
    void unchanged(const SettlementService& s) const {
        EXPECT_EQ(s.goodsBalance(bakery, wheat), Quantity(10));
        EXPECT_EQ(s.goodsBalance(bakery, fuel), Quantity(5));
        EXPECT_EQ(s.goodsBalance(bakery, bread), Quantity(0));
        EXPECT_EQ(s.availableLabor(bakery), Quantity(10));
        EXPECT_EQ(s.stateVersion(), 0);
    }
};
} // namespace

TEST_F(GoodsProcessingTest, ProcessingCommitsInputsOutputsAndLaborTogether) {
    SettlementService s(registry, {{registry.accountFor(owner).id, Money(100)}}, {}, setup());
    const auto r = s.execute(command());
    ASSERT_EQ(r.status, SettlementStatus::Succeeded);
    EXPECT_EQ(r.amount, Money(0));
    EXPECT_EQ(s.goodsBalance(bakery, wheat), Quantity(6));
    EXPECT_EQ(s.goodsBalance(bakery, fuel), Quantity(3));
    EXPECT_EQ(s.goodsBalance(bakery, bread), Quantity(6));
    EXPECT_EQ(s.availableLabor(bakery), Quantity(6));
    EXPECT_EQ(s.goodsBalance(owner, wheat), Quantity(100));
    EXPECT_EQ(s.totalCash(), Money(100));
    EXPECT_EQ(s.execute(command()).version_after, r.version_after);
    EXPECT_EQ(s.processedCount(), 1U);
    EXPECT_EQ(s.goodsBalance(bakery, bread), Quantity(6));
}

TEST_F(GoodsProcessingTest, LateInputShortageDoesNotConsumeEarlierInputOrLabor) {
    auto goods = setup();
    goods.balances.at({bakery, fuel}) = Quantity(1);
    SettlementService s(registry, {}, {}, goods);
    EXPECT_EQ(s.execute(command()).reason, SettlementReason::InsufficientGoods);
    EXPECT_EQ(s.goodsBalance(bakery, wheat), Quantity(10));
    EXPECT_EQ(s.goodsBalance(bakery, fuel), Quantity(1));
    EXPECT_EQ(s.goodsBalance(bakery, bread), Quantity(0));
    EXPECT_EQ(s.availableLabor(bakery), Quantity(10));
    EXPECT_EQ(s.execute(command()).reason, SettlementReason::InsufficientGoods);
    EXPECT_EQ(s.processedCount(), 1U);
    EXPECT_EQ(s.stateVersion(), 0);
}

TEST_F(GoodsProcessingTest, OutputOverflowRollsBackInputsAndLabor) {
    auto goods = setup();
    goods.balances.emplace(std::make_pair(bakery, bread), Quantity(std::numeric_limits<std::int64_t>::max()));
    SettlementService s(registry, {}, {}, goods);
    EXPECT_EQ(s.execute(command()).reason, SettlementReason::AmountOverflow);
    EXPECT_EQ(s.goodsBalance(bakery, wheat), Quantity(10));
    EXPECT_EQ(s.goodsBalance(bakery, fuel), Quantity(5));
    EXPECT_EQ(s.goodsBalance(bakery, bread), Quantity(std::numeric_limits<std::int64_t>::max()));
    EXPECT_EQ(s.availableLabor(bakery), Quantity(10));
    EXPECT_EQ(s.stateVersion(), 0);
}

TEST_F(GoodsProcessingTest, QuantityMultiplicationAndLaborLimitsAreChecked) {
    SettlementService s(registry, {}, {}, setup());
    EXPECT_EQ(s.execute(command(1, 0, std::numeric_limits<std::int64_t>::max())).reason,
              SettlementReason::AmountOverflow);
    EXPECT_EQ(s.execute(command(2, 0, 6)).reason, SettlementReason::InsufficientLabor);
    unchanged(s);
}

TEST_F(GoodsProcessingTest, RejectsWrongOwnerActorRecipePhaseVersionAndZeroBatches) {
    SettlementService s(registry, {}, {}, setup());
    auto wrongOwner = command(); wrongOwner.controller_person_id = std::nullopt;
    EXPECT_EQ(s.execute(wrongOwner).reason, SettlementReason::Unauthorized);
    auto household = command(2); household.actor_id = owner;
    EXPECT_EQ(s.execute(household).reason, SettlementReason::Unauthorized);
    auto recipe = command(3); recipe.recipe_id = "missing";
    EXPECT_EQ(s.execute(recipe).reason, SettlementReason::InvalidReference);
    auto phase = GoodsProcessingCommand{CommandId(4), std::string(64, 'a'), 0,
        GameTime(1, 7, 4), bakery, owner, "bread", Quantity(2)};
    EXPECT_EQ(s.execute(phase).reason, SettlementReason::InvalidRequest);
    EXPECT_EQ(s.execute(command(5, 1)).reason, SettlementReason::StaleState);
    EXPECT_EQ(s.execute(command(6, 0, 0)).reason, SettlementReason::InvalidRequest);
    registry.closeBusiness(bakery);
    EXPECT_EQ(s.execute(command(7)).reason, SettlementReason::InactiveActor);
    unchanged(s);
}

TEST_F(GoodsProcessingTest, CashAndGoodsShareCommandIdentityVersionAndTime) {
    const auto from = registry.accountFor(owner).id;
    const auto to = registry.accountFor(bakery).id;
    SettlementService s(registry, {{from, Money(10)}}, {}, setup());
    ASSERT_EQ(s.execute(command()).status, SettlementStatus::Succeeded);
    const CashTransferCommand conflict{CommandId(1), std::string(64, 'a'), 0,
        GameTime(1, 6, 1), owner, owner, from, {{to, Money(1)}}};
    EXPECT_EQ(s.execute(conflict).reason, SettlementReason::IdConflict);
    const CashTransferCommand cash{CommandId(2), std::string(64, 'a'), 1,
        GameTime(1, 6, 2), owner, owner, from, {{to, Money(1)}}};
    EXPECT_EQ(s.execute(cash).status, SettlementStatus::Succeeded);
    EXPECT_EQ(s.execute(command(3, 2)).status, SettlementStatus::Succeeded);
    auto changed = command(); changed.batches = Quantity(1);
    EXPECT_EQ(s.execute(changed).reason, SettlementReason::IdConflict);
    EXPECT_EQ(s.balance(from), Money(9));
    EXPECT_EQ(s.totalCash(), Money(10));
    EXPECT_EQ(s.stateVersion(), 3);
}

TEST_F(GoodsProcessingTest, ExceptionsNeverPublishInventoryLaborOrResult) {
    for (const auto failAt : {SettlementStage::AfterInput, SettlementStage::AfterOutput,
                              SettlementStage::BeforePublish}) {
        bool fail = true;
        SettlementService s(registry, {}, [&](SettlementStage stage) {
            if (fail && stage == failAt) throw std::overflow_error("injected");
        }, setup());
        EXPECT_THROW(s.execute(command()), std::overflow_error);
        unchanged(s);
        EXPECT_EQ(s.processedCount(), 0U);
        fail = false;
        EXPECT_EQ(s.execute(command()).status, SettlementStatus::Succeeded);
    }
}

TEST_F(GoodsProcessingTest, RejectsInvalidOpeningConfiguration) {
    auto goods = setup(); goods.products.erase(bread);
    EXPECT_THROW((SettlementService(registry, {}, {}, goods)), std::invalid_argument);
    goods = setup(); goods.recipes.at("bread").labor_per_batch = Quantity(0);
    EXPECT_THROW((SettlementService(registry, {}, {}, goods)), std::invalid_argument);
    goods = setup(); goods.balances.emplace(std::make_pair(ActorId(999), wheat), Quantity(1));
    EXPECT_THROW((SettlementService(registry, {}, {}, goods)), std::out_of_range);
}
