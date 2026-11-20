#include "decision/command_gateway.h"

#include <gtest/gtest.h>

using namespace economy;

namespace {
const std::string hash(64, 'b');

const ActorRegistry& registry() {
    static const auto value = [] {
        std::vector<ActorSpec> specs;
        for (int i = 1; i <= 17; ++i) {
            specs.emplace_back("person:" + std::to_string(100 + i), "person", ActorKind::Person);
        }
        specs.emplace_back("z:lender", "lender", ActorKind::Lender);
        specs.emplace_back("z:government", "government", ActorKind::Government);
        return ActorRegistry::fromSpecs(specs);
    }();
    return value;
}

DecisionRequest request(unsigned int phase = 8) {
    return DecisionRequest("run-42", "day8-phase08-actor17-food-attempt1", hash, ActorId(17), ActorId(17),
                           GameTime(8, phase, 1), 4,
                           {{"pass", PassAction{"NO_ACTION"}},
                            {"buy", PlaceOrderAction{IntentId(91), QuoteId(40), Quantity(3), Money(30), true}}});
}

DecisionResponse response(const std::string& candidate_id = "buy") {
    return {"decision.v1", "day8-phase08-actor17-food-attempt1", hash,
            candidate_id, ReasonCode::FoodPriority};
}
} // namespace

TEST(CommandGatewayTest, CopiesStoredOrderAndTrustedMetadata) {
    const auto prepared = CommandGateway::prepareOrder(registry(), request(), response(),
        "run-42", "run-42",
        ActorId(17), ActorId(17), 4, [] { return CommandId(100); });
    ASSERT_TRUE(prepared.has_value());
    EXPECT_EQ(prepared->command_id.value(), 100U);
    EXPECT_EQ(prepared->run_id, "run-42");
    EXPECT_EQ(prepared->decision_id, "day8-phase08-actor17-food-attempt1");
    EXPECT_EQ(prepared->actor_id.value(), 17U);
    ASSERT_TRUE(prepared->controller_person_id.has_value());
    EXPECT_EQ(prepared->controller_person_id->value(), 17U);
    EXPECT_EQ(prepared->time.day, 8U);
    EXPECT_EQ(prepared->time.phase, 8U);
    EXPECT_EQ(prepared->expected_state_version, 4U);
    EXPECT_EQ(prepared->input_hash, hash);
    EXPECT_EQ(prepared->order.intent_id.value(), 91U);
    EXPECT_EQ(prepared->order.quote_id.value(), 40U);
    EXPECT_EQ(prepared->order.quantity.value(), 3);
    EXPECT_EQ(prepared->order.max_amount.value(), 30);
    EXPECT_TRUE(prepared->order.allow_partial);
}

TEST(CommandGatewayTest, PassDoesNotProduceACommand) {
    int allocations = 0;
    const auto prepared = CommandGateway::prepareOrder(registry(), request(), response("pass"),
        "run-42", "run-42",
        ActorId(17), ActorId(17), 4, [&] { ++allocations; return CommandId(100); });
    EXPECT_FALSE(prepared.has_value());
    EXPECT_EQ(allocations, 0);
}

TEST(CommandGatewayTest, RejectsCallerStatePhaseAndUnknownCandidate) {
    int allocations = 0;
    const auto allocate = [&] { ++allocations; return CommandId(100); };
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(), response(),
        "other-run", "run-42",
        ActorId(17), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(), response(),
        "run-42", "other-run",
        ActorId(17), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(), response(),
        "run-42", "run-42",
        ActorId(18), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(), response(),
        "run-42", "run-42",
        ActorId(17), std::nullopt, 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(), response(),
        "run-42", "run-42",
        ActorId(17), ActorId(17), 5, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(3), response(),
        "run-42", "run-42",
        ActorId(17), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(registry(), request(), response("unknown"),
        "run-42", "run-42",
        ActorId(17), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_EQ(allocations, 0);
}

TEST(CommandGatewayTest, ChecksRegisteredOwnerBeforeAllocatingEvenForPass) {
    auto actors = ActorRegistry::fromSpecs({
        {"owner", "owner", ActorKind::Person},
        {"other", "other", ActorKind::Person},
        {"shop", "shop", ActorKind::Business, "owner", true},
        {"lender", "lender", ActorKind::Lender},
        {"government", "government", ActorKind::Government}
    });
    const auto owner = actors.actorByKey("owner").id;
    const auto other = actors.actorByKey("other").id;
    const auto shop = actors.actorByKey("shop").id;
    const auto government = actors.actorByKey("government").id;
    int allocations = 0;
    const auto prepare = [&](ActorId actor, std::optional<ActorId> controller,
                             const std::string& candidate = "buy") {
        const DecisionRequest input("run-42", response().decision_id, hash, actor, controller,
            GameTime(8, 8, 1), 4, request().candidates());
        return CommandGateway::prepareOrder(actors, input, response(candidate),
            "run-42", "run-42", actor, controller, 4,
            [&] { return CommandId(++allocations); });
    };
    EXPECT_THROW(prepare(shop, other), std::invalid_argument);
    EXPECT_THROW(prepare(owner, other), std::invalid_argument);
    EXPECT_THROW(prepare(owner, std::nullopt), std::invalid_argument);
    EXPECT_THROW(prepare(government, owner), std::invalid_argument);
    EXPECT_THROW(prepare(ActorId(999), std::nullopt), std::out_of_range);
    EXPECT_EQ(allocations, 0);
    ASSERT_TRUE(prepare(shop, owner).has_value());
    EXPECT_EQ(allocations, 1);
    ASSERT_TRUE(prepare(government, std::nullopt).has_value());
    EXPECT_EQ(allocations, 2);
    actors.closeBusiness(shop);
    EXPECT_THROW(prepare(shop, owner), std::invalid_argument);
    EXPECT_THROW(prepare(shop, owner, "pass"), std::invalid_argument);
    EXPECT_EQ(allocations, 2);
}
