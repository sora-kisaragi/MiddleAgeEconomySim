#include "core/settlement_service.h"
#include <gtest/gtest.h>
#include <limits>

using namespace economy;
namespace {
class GoodsFlowTest : public ::testing::Test {
protected:
    ActorRegistry registry = ActorRegistry::fromSpecs({
        {"person", "person", ActorKind::Person},
        {"mill", "mill", ActorKind::Business, "person"},
        {"lender", "lender", ActorKind::Lender},
        {"government", "government", ActorKind::Government}});
    ActorId person = registry.actorByKey("person").id;
    ActorId mill = registry.actorByKey("mill").id;
    ProductId wheat{1}, bread{2};
    GoodsSetup setup() const {
        return {{wheat, bread}, {{"bread", {{{wheat, Quantity(2)}},
            {{bread, Quantity(3)}}, Quantity(1)}}},
            {{{mill, wheat}, Quantity(10)}}, {{mill, Quantity(5)}}};
    }
    GoodsFlowCommand flow(std::uint64_t id, std::uint64_t version,
                          GoodsFlowKind kind, ActorId actor, ProductId product,
                          std::int64_t amount, std::optional<ActorId> to = {}) const {
        return {CommandId(id), std::string(64, 'a'), version, GameTime(1, 6, id),
                actor, person, kind, product, Quantity(amount), to};
    }
};
}

TEST_F(GoodsFlowTest, ReconstructsWholeLifecycleFromOpeningAndCommittedEntries) {
    auto opening = setup();
    SettlementService s(registry, {{registry.accountFor(person).id, Money(100)}}, {}, opening);
    ASSERT_EQ(s.execute(flow(1, 0, GoodsFlowKind::Production, mill, wheat, 4)).status, SettlementStatus::Succeeded);
    GoodsProcessingCommand processing{CommandId(2), std::string(64, 'b'), 1,
        GameTime(1, 6, 2), mill, person, "bread", Quantity(2)};
    ASSERT_EQ(s.execute(processing).status, SettlementStatus::Succeeded);
    auto transfer = flow(3, 2, GoodsFlowKind::Transfer, mill, bread, 5, person);
    ASSERT_EQ(s.execute(transfer).status, SettlementStatus::Succeeded);
    ASSERT_EQ(s.execute(flow(4, 3, GoodsFlowKind::Consumption, person, bread, 3)).status, SettlementStatus::Succeeded);
    ASSERT_EQ(s.execute(flow(5, 4, GoodsFlowKind::Disposal, person, bread, 1)).status, SettlementStatus::Succeeded);
    EXPECT_EQ(s.goodsBalance(mill, wheat), Quantity(10));
    EXPECT_EQ(s.goodsBalance(mill, bread), Quantity(1));
    EXPECT_EQ(s.goodsBalance(person, bread), Quantity(1));
    EXPECT_EQ(s.totalCash(), Money(100));
    EXPECT_EQ(s.availableLabor(mill), Quantity(3));
    const auto history = s.goodsHistory();
    ASSERT_EQ(history.size(), 7U);
    // Independent signed replay, including absent opening rows.
    std::map<std::pair<ActorId, ProductId>, std::int64_t> replay;
    for (const auto& row : opening.balances) replay[row.first] = row.second.value();
    for (const auto& row : history) {
        const bool incoming = row.kind == GoodsEntryKind::Production || row.kind == GoodsEntryKind::Receipt;
        replay[{row.actor_id, row.product_id}] += (incoming ? 1 : -1) * row.quantity.value();
    }
    for (auto actor : {mill, person}) for (auto product : {wheat, bread}) {
        EXPECT_EQ(s.goodsBalance(actor, product).value(), replay[std::make_pair(actor, product)]);
    }
    EXPECT_EQ(s.execute(transfer).version_after, 3U);
    EXPECT_EQ(s.goodsHistory().size(), 7U);
    auto snapshot = s.goodsHistory(); snapshot.clear();
    EXPECT_EQ(s.goodsHistory().size(), 7U);
}

TEST_F(GoodsFlowTest, RejectsLateOverflowWithoutPublishingDeliveryOrHistory) {
    auto goods = setup(); goods.balances.insert_or_assign({person, wheat}, Quantity(std::numeric_limits<std::int64_t>::max()));
    SettlementService s(registry, {}, {}, goods);
    auto c = flow(1, 0, GoodsFlowKind::Transfer, mill, wheat, 1, person);
    EXPECT_EQ(s.execute(c).reason, SettlementReason::AmountOverflow);
    EXPECT_EQ(s.execute(c).reason, SettlementReason::AmountOverflow);
    EXPECT_EQ(s.goodsBalance(mill, wheat), Quantity(10));
    EXPECT_EQ(s.goodsBalance(person, wheat), Quantity(std::numeric_limits<std::int64_t>::max()));
    EXPECT_TRUE(s.goodsHistory().empty());
    EXPECT_EQ(s.processedCount(), 1U);
    EXPECT_EQ(s.stateVersion(), 0U);
}

TEST_F(GoodsFlowTest, ValidatesAuthorityReferencesQuantityAndKind) {
    SettlementService s(registry, {}, {}, setup());
    {
        auto c = flow(1, 0, GoodsFlowKind::Transfer, mill, wheat, 1, ActorId(999));
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidReference);
    }
    {
        auto c = flow(2, 0, GoodsFlowKind::Transfer, mill, wheat, 1, mill);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidRequest);
    }
    {
        auto c = flow(3, 0, GoodsFlowKind::Consumption, mill, wheat, 11);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InsufficientGoods);
    }
    {
        auto c = flow(4, 0, GoodsFlowKind::Production, person, wheat, 1);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::Unauthorized);
    }
    {
        auto c = flow(5, 0, GoodsFlowKind::Disposal, mill, wheat, 1); c.controller_person_id = {};
        EXPECT_EQ(s.execute(c).reason, SettlementReason::Unauthorized);
    }
    {
        auto c = flow(6, 0, GoodsFlowKind::Consumption, person, ProductId(999), 1);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidReference);
    }
    {
        auto c = flow(7, 0, GoodsFlowKind::Disposal, mill, wheat, 0);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidRequest);
    }
    {
        auto c = flow(8, 0, static_cast<GoodsFlowKind>(99), mill, wheat, 1);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidRequest);
    }
    EXPECT_TRUE(s.goodsHistory().empty());
    EXPECT_EQ(s.goodsBalance(mill, wheat), Quantity(10));
}

TEST_F(GoodsFlowTest, ExceptionsRollbackBothAssetsAndHistoryAndAllowRetry) {
    for (auto stage : {SettlementStage::AfterInput, SettlementStage::AfterOutput, SettlementStage::BeforePublish}) {
        bool fail = true;
        SettlementService s(registry, {}, [&](SettlementStage point) {
            if (fail && point == stage) throw std::bad_alloc();
        }, setup());
        auto c = flow(1, 0, GoodsFlowKind::Transfer, mill, wheat, 2, person);
        EXPECT_THROW(s.execute(c), std::bad_alloc);
        EXPECT_EQ(s.goodsBalance(mill, wheat), Quantity(10));
        EXPECT_EQ(s.goodsBalance(person, wheat), Quantity(0));
        EXPECT_EQ(s.stateVersion(), 0U);
        EXPECT_EQ(s.processedCount(), 0U);
        EXPECT_TRUE(s.goodsHistory().empty());
        fail = false;
        EXPECT_EQ(s.execute(c).status, SettlementStatus::Succeeded);
        EXPECT_EQ(s.goodsHistory().size(), 2U);
    }
}

TEST_F(GoodsFlowTest, SharesIdentityAcrossCommandsAndNeverRepeatsConsumption) {
    SettlementService s(registry, {}, {}, setup());
    auto c = flow(1, 0, GoodsFlowKind::Consumption, mill, wheat, 2);
    ASSERT_EQ(s.execute(c).status, SettlementStatus::Succeeded);
    EXPECT_EQ(s.execute(c).status, SettlementStatus::Succeeded);
    c.quantity = Quantity(1);
    EXPECT_EQ(s.execute(c).reason, SettlementReason::IdConflict);
    GoodsProcessingCommand processing{CommandId(1), std::string(64, 'a'), 0,
        GameTime(1, 6, 1), mill, person, "bread", Quantity(1)};
    EXPECT_EQ(s.execute(processing).reason, SettlementReason::IdConflict);
    EXPECT_EQ(s.goodsBalance(mill, wheat), Quantity(8));
    EXPECT_EQ(s.goodsHistory().size(), 1U);
}

TEST_F(GoodsFlowTest, RejectsMalformedRecipientsOverflowAndInactiveSources) {
    SettlementService s(registry, {}, {}, setup());
    {
        auto c = flow(1, 0, GoodsFlowKind::Transfer, mill, wheat, 1);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidRequest);
    }
    {
        auto c = flow(2, 0, GoodsFlowKind::Production, mill, wheat, 1, person);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InvalidRequest);
    }
    {
        auto c = flow(3, 0, GoodsFlowKind::Production, mill, wheat, std::numeric_limits<std::int64_t>::max());
        EXPECT_EQ(s.execute(c).reason, SettlementReason::AmountOverflow);
    }
    {
        auto c = flow(4, 1, GoodsFlowKind::Consumption, mill, wheat, 1);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::StaleState);
    }
    registry.closeBusiness(mill);
    {
        auto c = flow(5, 0, GoodsFlowKind::Consumption, mill, wheat, 1);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InactiveActor);
    }
    {
        auto c = flow(6, 0, GoodsFlowKind::Transfer, person, wheat, 1, mill);
        EXPECT_EQ(s.execute(c).reason, SettlementReason::InactiveActor);
    }
    EXPECT_EQ(s.goodsBalance(mill, wheat), Quantity(10));
    EXPECT_TRUE(s.goodsHistory().empty());
    EXPECT_EQ(s.stateVersion(), 0U);
}
