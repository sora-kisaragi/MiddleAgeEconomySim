#pragma once

#include "decision/command_gateway.h"

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace economy {

// A policy proposes a candidate ID; it never receives a mutable registry or
// a settlement service. Implementations must not use this interface to commit
// balances, inventory, employment, or contracts.
class IDecisionPolicy {
public:
    virtual ~IDecisionPolicy() = default;
    virtual DecisionResponse decide(const DecisionRequest& request) const = 0;
};

enum class DecisionOutcome { Passed, OrderPrepared };

// This describes a policy decision, NOT a committed settlement. In particular,
// OrderPrepared must not be reported as a successful transaction.
struct DecisionResult {
    std::string decision_id;
    std::string candidate_id;
    ReasonCode reason_code;
    DecisionOutcome outcome;
    std::optional<PreparedOrderCommand> prepared_order;
};

// Invoke exactly one policy and pass its response through the existing
// authorization and candidate-selection gateway. Invalid decisions propagate
// as exceptions before command ID allocation. Only a trusted caller may bind
// the execution run IDs and allocate command IDs.
inline DecisionResult evaluateDecision(
    const IDecisionPolicy& policy, const ActorRegistry& registry,
    const DecisionRequest& request, const std::string& caller_run_id,
    const std::string& response_run_id, ActorId caller_actor_id,
    std::optional<ActorId> caller_controller_person_id,
    std::uint64_t current_state_version,
    const std::function<CommandId()>& allocate_command_id) {
    const DecisionResponse response = policy.decide(request);
    auto prepared = CommandGateway::prepareOrder(
        registry, request, response, caller_run_id, response_run_id,
        caller_actor_id, caller_controller_person_id, current_state_version,
        allocate_command_id);
    const auto outcome = prepared ? DecisionOutcome::OrderPrepared : DecisionOutcome::Passed;
    return DecisionResult{request.decisionId(), response.candidate_id,
                          response.reason_code, outcome, std::move(prepared)};
}

} // namespace economy
