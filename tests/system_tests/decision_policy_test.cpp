#include "decision/decision_policy.h"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace economy;

namespace {
const std::string hash(64, 'a');

const ActorRegistry& actors() {
    static const auto value = ActorRegistry::fromSpecs({
        {"person", "person", ActorKind::Person},
        {"lender", "lender", ActorKind::Lender},
        {"government", "government", ActorKind::Government}
    });
    return value;
}

DecisionRequest request() {
    const auto person = actors().actorByKey("person").id;
    return DecisionRequest("run-42", "decision-1", hash, person, person,
        GameTime(1, 8, 1), 0,
        {{"pass", PassAction{"NO_ACTION"}},
         {"buy", PlaceOrderAction{IntentId(1), QuoteId(1), Quantity(2), Money(20), true}}});
}

class FixedPolicy final : public IDecisionPolicy {
public:
    explicit FixedPolicy(std::string selection) : selection_(std::move(selection)) {}
    DecisionResponse decide(const DecisionRequest& input) const override {
        ++invocations;
        return {"decision.v1", input.decisionId(), input.inputHash(),
                selection_, ReasonCode::FoodPriority};
    }
    mutable int invocations = 0;
private:
    std::string selection_;
};

class InvalidPolicy final : public IDecisionPolicy {
public:
    DecisionResponse decide(const DecisionRequest& input) const override {
        return {"decision.v1", "wrong-decision", input.inputHash(),
                "buy", ReasonCode::FoodPriority};
    }
};
} // namespace

TEST(DecisionPolicyTest, PassDoesNotAllocateOrSettle) {
    const FixedPolicy policy("pass");
    int allocations = 0;
    const auto input = request();
    const auto person = input.actorId();
    const auto result = evaluateDecision(policy, actors(), input, "run-42", "run-42",
        person, person, 0, [&] { ++allocations; return CommandId(1); });
    EXPECT_EQ(policy.invocations, 1);
    EXPECT_EQ(allocations, 0);
    EXPECT_EQ(result.decision_id, "decision-1");
    EXPECT_EQ(result.candidate_id, "pass");
    EXPECT_EQ(result.outcome, DecisionOutcome::Passed);
    EXPECT_FALSE(result.prepared_order.has_value());
}

TEST(DecisionPolicyTest, SelectedOrderIsOnlyPreparedOnce) {
    const FixedPolicy policy("buy");
    int allocations = 0;
    const auto input = request();
    const auto person = input.actorId();
    const auto result = evaluateDecision(policy, actors(), input, "run-42", "run-42",
        person, person, 0, [&] { return CommandId(++allocations); });
    EXPECT_EQ(policy.invocations, 1);
    EXPECT_EQ(allocations, 1);
    EXPECT_EQ(result.outcome, DecisionOutcome::OrderPrepared);
    ASSERT_TRUE(result.prepared_order.has_value());
    EXPECT_EQ(result.prepared_order->command_id.value(), 1U);
    EXPECT_EQ(result.prepared_order->order.quantity.value(), 2);
    EXPECT_EQ(result.prepared_order->order.max_amount.value(), 20);
}

TEST(DecisionPolicyTest, RejectsUntrustedOrStalePolicyResultBeforeAllocating) {
    const InvalidPolicy policy;
    const auto input = request();
    const auto person = input.actorId();
    int allocations = 0;
    const auto allocate = [&] { ++allocations; return CommandId(1); };
    EXPECT_THROW(evaluateDecision(policy, actors(), input, "run-42", "run-42",
        person, person, 0, allocate), std::invalid_argument);
    const FixedPolicy valid("buy");
    EXPECT_THROW(evaluateDecision(valid, actors(), input, "run-42", "other-run",
        person, person, 0, allocate), std::invalid_argument);
    EXPECT_THROW(evaluateDecision(valid, actors(), input, "run-42", "run-42",
        person, person, 1, allocate), std::invalid_argument);
    EXPECT_EQ(allocations, 0);
}
