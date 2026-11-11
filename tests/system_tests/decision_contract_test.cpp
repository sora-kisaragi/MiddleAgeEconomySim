#include "decision/decision_contract.h"

#include <gtest/gtest.h>

using namespace economy;

namespace {
const std::string hash(64, 'a');

DecisionRequest request() {
    return DecisionRequest("run-42", "day8-phase08-actor17-food-attempt1", hash, ActorId(17), ActorId(17),
                           GameTime(8, 8, 1), 4,
                           {{"pass", PassAction{"NO_ACTION"}},
                            {"buy", PlaceOrderAction{IntentId(91), QuoteId(40), Quantity(3), Money(30), true}}});
}

DecisionResponse response() {
    return {"decision.v1", "day8-phase08-actor17-food-attempt1", hash, "buy", ReasonCode::FoodPriority};
}
} // namespace

TEST(DecisionContractTest, SelectsOnlyTheStoredAction) {
    const auto selection = selectCandidate(request(), response(), 4);
    const auto& order = std::get<PlaceOrderAction>(selection.action);
    EXPECT_EQ(order.intent_id.value(), 91U);
    EXPECT_EQ(order.quote_id.value(), 40U);
    EXPECT_EQ(order.quantity.value(), 3);
    EXPECT_EQ(order.max_amount.value(), 30);
}

TEST(DecisionContractTest, RejectsMismatchedAndUnknownResponses) {
    auto selected = response();
    selected.decision_id = "another";
    EXPECT_THROW(selectCandidate(request(), selected, 4), std::invalid_argument);
    selected = response();
    selected.input_hash[0] = 'b';
    EXPECT_THROW(selectCandidate(request(), selected, 4), std::invalid_argument);
    selected = response();
    selected.candidate_id = "missing";
    EXPECT_THROW(selectCandidate(request(), selected, 4), std::invalid_argument);
    selected = response();
    selected.schema_version = "decision.v2";
    EXPECT_THROW(selectCandidate(request(), selected, 4), std::invalid_argument);
    selected = response();
    selected.reason_code = static_cast<ReasonCode>(99);
    EXPECT_THROW(selectCandidate(request(), selected, 4), std::invalid_argument);
    selected.reason_code = static_cast<ReasonCode>(-1);
    EXPECT_THROW(selectCandidate(request(), selected, 4), std::invalid_argument);
    EXPECT_THROW(selectCandidate(request(), response(), 5), std::invalid_argument);
}

TEST(DecisionContractTest, RejectsDuplicateCandidatesAndMalformedIdentity) {
    EXPECT_THROW((DecisionRequest("run-42", "d", hash, ActorId(1), std::nullopt, GameTime(1, 2, 0), 0,
                                  {{"same", PassAction{"NO_ACTION"}}, {"same", PassAction{"NO_ACTION"}}})),
                 std::invalid_argument);
    EXPECT_THROW((DecisionRequest("run-42", "d", hash, ActorId(1), std::nullopt, GameTime(1, 2, 0), 0,
                                  {{"order", PlaceOrderAction{IntentId(1), QuoteId(1), Quantity(1), Money(1), false}}})),
                 std::invalid_argument);
    EXPECT_THROW((DecisionRequest("run-42", "d", std::string(64, 'G'), ActorId(1), std::nullopt,
                                  GameTime(1, 2, 0), 0, {{"pass", PassAction{"NO_ACTION"}}})),
                 std::invalid_argument);
    EXPECT_THROW((DecisionRequest("run-42", "d", hash, ActorId(1), std::nullopt, GameTime(1, 2, 0), 0,
                                  {{"pass", PassAction{""}}})), std::invalid_argument);
    EXPECT_THROW((DecisionRequest("", "d", hash, ActorId(1), std::nullopt, GameTime(1, 2, 0), 0,
                                  {{"pass", PassAction{"NO_ACTION"}}})), std::invalid_argument);
}

TEST(DecisionContractTest, AllowsZeroBudgetForFreeQuotesButRequiresQuantity) {
    const DecisionRequest free_order("run-42", "free-order", hash, ActorId(1), std::nullopt,
                                    GameTime(1, 8, 0), 0,
                                    {{"pass", PassAction{"NO_ACTION"}},
                                     {"free", PlaceOrderAction{IntentId(1), QuoteId(1),
                                                               Quantity(2), Money(0), false}}});
    const auto selected = selectCandidate(free_order,
        {"decision.v1", "free-order", hash, "free", ReasonCode::FoodPriority}, 0);
    EXPECT_EQ(std::get<PlaceOrderAction>(selected.action).max_amount.value(), 0);
    EXPECT_THROW((DecisionRequest("run-42", "invalid-order", hash, ActorId(1), std::nullopt,
                                  GameTime(1, 8, 0), 0,
                                  {{"pass", PassAction{"NO_ACTION"}},
                                   {"zero", PlaceOrderAction{IntentId(1), QuoteId(1),
                                                              Quantity(0), Money(0), false}}})),
                 std::invalid_argument);
}
