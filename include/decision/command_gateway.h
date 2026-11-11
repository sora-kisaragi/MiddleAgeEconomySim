#pragma once

#include "decision/decision_contract.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace economy {

// Prepared data for the future settlement command. It cannot change balances,
// inventory, contracts or state versions on its own.
struct PreparedOrderCommand {
    const CommandId command_id;
    const std::string run_id;
    const std::string decision_id;
    const ActorId actor_id;
    const std::optional<ActorId> controller_person_id;
    const GameTime time;
    const std::uint64_t expected_state_version;
    const std::string input_hash;
    const PlaceOrderAction order;
};

class CommandGateway {
public:
    // Caller and transport run IDs, identity, state version and the ID allocator come from the
    // trusted driver/registry, never from the policy response. The allocator
    // runs only after a valid non-PASS candidate and phase are established.
    // The caller must bind response_run_id to the actual invocation; this
    // function cannot authenticate a caller-supplied string. Registry authorization,
    // canonical request hashing and settlement validation remain mandatory.
    static std::optional<PreparedOrderCommand> prepareOrder(
        const DecisionRequest& request, const DecisionResponse& response,
        const std::string& caller_run_id, const std::string& response_run_id,
        ActorId caller_actor_id, std::optional<ActorId> caller_controller_person_id,
        std::uint64_t current_state_version, const std::function<CommandId()>& allocate_command_id) {
        if (caller_run_id != request.runId() || response_run_id != request.runId() ||
            caller_actor_id != request.actorId() ||
            caller_controller_person_id != request.controllerPersonId()) {
            throw std::invalid_argument("decision caller does not match request");
        }
        const auto selected = selectCandidate(request, response, current_state_version);
        if (std::holds_alternative<PassAction>(selected.action)) return std::nullopt;
        const auto phase = request.time().phase;
        if (phase != 5 && phase != 7 && phase != 8) {
            throw std::invalid_argument("PLACE_ORDER is not permitted in this phase");
        }
        if (!allocate_command_id) throw std::invalid_argument("command ID allocator is required");
        return PreparedOrderCommand{allocate_command_id(), request.runId(), request.decisionId(), request.actorId(),
            request.controllerPersonId(), request.time(), request.stateVersion(),
            request.inputHash(), std::get<PlaceOrderAction>(selected.action)};
    }
};

} // namespace economy
