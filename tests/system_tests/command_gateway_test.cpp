#include "decision/command_gateway.h"

#include <gtest/gtest.h>

using namespace economy;

namespace {
const std::string hash(64, 'b');

DecisionRequest request(unsigned int phase = 8) {
    return DecisionRequest("day8-phase08-actor17-food-attempt1", hash, ActorId(17), ActorId(17),
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
    const auto prepared = CommandGateway::prepareOrder(request(), response(),
        ActorId(17), ActorId(17), 4, [] { return CommandId(100); });
    ASSERT_TRUE(prepared.has_value());
    EXPECT_EQ(prepared->command_id.value(), 100U);
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
    const auto prepared = CommandGateway::prepareOrder(request(), response("pass"),
        ActorId(17), ActorId(17), 4, [&] { ++allocations; return CommandId(100); });
    EXPECT_FALSE(prepared.has_value());
    EXPECT_EQ(allocations, 0);
}

TEST(CommandGatewayTest, RejectsCallerStatePhaseAndUnknownCandidate) {
    int allocations = 0;
    const auto allocate = [&] { ++allocations; return CommandId(100); };
    EXPECT_THROW(CommandGateway::prepareOrder(request(), response(),
        ActorId(18), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(request(), response(),
        ActorId(17), std::nullopt, 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(request(), response(),
        ActorId(17), ActorId(17), 5, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(request(3), response(),
        ActorId(17), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_THROW(CommandGateway::prepareOrder(request(), response("unknown"),
        ActorId(17), ActorId(17), 4, allocate), std::invalid_argument);
    EXPECT_EQ(allocations, 0);
}
