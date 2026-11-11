#pragma once

#include "core/economy_types.h"

#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace economy {

struct QuoteTag {};
using QuoteId = Id<QuoteTag>;

struct PassAction {
    std::string reason;
};

struct PlaceOrderAction {
    IntentId intent_id;
    QuoteId quote_id;
    Quantity quantity;
    Money max_amount;
    bool allow_partial;
};

using Action = std::variant<PassAction, PlaceOrderAction>;

struct Candidate {
    std::string id;
    Action action;
};

inline bool validDecisionId(const std::string& id) {
    std::size_t characters = 0;
    for (std::size_t i = 0; i < id.size();) {
        const auto lead = static_cast<unsigned char>(id[i++]);
        std::uint32_t cp = lead;
        unsigned int continuation = 0;
        std::uint32_t minimum = 0;
        if (lead >= 0xc2 && lead <= 0xdf) {
            cp = lead & 0x1f; continuation = 1; minimum = 0x80;
        } else if (lead >= 0xe0 && lead <= 0xef) {
            cp = lead & 0x0f; continuation = 2; minimum = 0x800;
        } else if (lead >= 0xf0 && lead <= 0xf4) {
            cp = lead & 0x07; continuation = 3; minimum = 0x10000;
        } else if (lead >= 0x80) {
            return false;
        }
        if (i + continuation > id.size()) return false;
        for (unsigned int n = 0; n < continuation; ++n) {
            const auto tail = static_cast<unsigned char>(id[i++]);
            if ((tail & 0xc0) != 0x80) return false;
            cp = (cp << 6) | (tail & 0x3f);
        }
        if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff) ||
            cp < 0x20 || cp == 0x7f || (cp >= 0x80 && cp <= 0x9f)) return false;
        if (++characters > 128) return false;
    }
    return characters != 0;
}

inline bool validHash(const std::string& hash) {
    return hash.size() == 64 && std::all_of(hash.begin(), hash.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

// The policy sees this immutable request and can return only a candidate ID.
// This is an in-process value contract, not a trust boundary: the caller still
// supplies input_hash. The ObservationBuilder must compute and verify the
// canonical request hash before this is exposed to an external policy.
// The remaining action kinds and serialized JSON contract are subsequent #89 work.
class DecisionRequest {
public:
    DecisionRequest(std::string run_id, std::string decision_id, std::string input_hash, ActorId actor_id,
                    std::optional<ActorId> controller_person_id, GameTime time,
                    std::uint64_t state_version, std::vector<Candidate> candidates)
        : run_id_(std::move(run_id)), decision_id_(std::move(decision_id)), input_hash_(std::move(input_hash)),
          actor_id_(actor_id), controller_person_id_(controller_person_id), time_(time),
          state_version_(state_version), candidates_(std::move(candidates)) {
        if (!validDecisionId(run_id_) || !validDecisionId(decision_id_) ||
            !validHash(input_hash_) || candidates_.empty()) {
            throw std::invalid_argument("invalid decision request identity or empty candidates");
        }
        std::set<std::string> ids;
        bool has_pass = false;
        for (const auto& candidate : candidates_) {
            if (!validDecisionId(candidate.id) || !ids.insert(candidate.id).second) {
                throw std::invalid_argument("candidate IDs must be unique and bounded");
            }
            if (const auto* pass = std::get_if<PassAction>(&candidate.action)) {
                if (pass->reason.empty()) throw std::invalid_argument("PASS needs a reason");
                has_pass = true;
            } else {
                const auto& order = std::get<PlaceOrderAction>(candidate.action);
                if (order.quantity.value() == 0) {
                    throw std::invalid_argument("order candidate must have positive quantity");
                }
            }
        }
        if (!has_pass) throw std::invalid_argument("decision needs a PASS candidate");
    }

    const std::string& runId() const noexcept { return run_id_; }
    const std::string& decisionId() const noexcept { return decision_id_; }
    const std::string& inputHash() const noexcept { return input_hash_; }
    ActorId actorId() const noexcept { return actor_id_; }
    std::optional<ActorId> controllerPersonId() const noexcept { return controller_person_id_; }
    const GameTime& time() const noexcept { return time_; }
    std::uint64_t stateVersion() const noexcept { return state_version_; }
    const std::vector<Candidate>& candidates() const noexcept { return candidates_; }

private:
    const std::string run_id_;
    const std::string decision_id_;
    const std::string input_hash_;
    const ActorId actor_id_;
    const std::optional<ActorId> controller_person_id_;
    const GameTime time_;
    const std::uint64_t state_version_;
    const std::vector<Candidate> candidates_;
};

enum class ReasonCode {
    FoodPriority, Preference, WorkIncome, Restock, Margin, CashReserve,
    FinanceGap, ProductionTarget, PriceTarget, OperatingCondition, NoAction
};

struct DecisionResponse {
    std::string schema_version;
    std::string decision_id;
    std::string input_hash;
    std::string candidate_id;
    ReasonCode reason_code;
};

// Return a copy of the candidate from the original request; no action
// arguments are accepted from a policy or reconstructed from its response.
inline Candidate selectCandidate(const DecisionRequest& request, const DecisionResponse& response,
                                 std::uint64_t current_state_version) {
    if (current_state_version != request.stateVersion() ||
        response.schema_version != "decision.v1" || response.decision_id != request.decisionId() ||
        response.input_hash != request.inputHash() || !validDecisionId(response.candidate_id) ||
        response.reason_code < ReasonCode::FoodPriority || response.reason_code > ReasonCode::NoAction) {
        throw std::invalid_argument("invalid decision response");
    }
    const auto it = std::find_if(request.candidates().begin(), request.candidates().end(),
                                 [&](const Candidate& c) { return c.id == response.candidate_id; });
    if (it == request.candidates().end()) throw std::invalid_argument("unknown candidate ID");
    return *it;
}

} // namespace economy
