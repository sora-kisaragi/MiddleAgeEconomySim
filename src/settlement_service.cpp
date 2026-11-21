#include "core/settlement_service.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace economy {
namespace {
bool sameRequest(const CashTransferCommand& lhs, const CashTransferCommand& rhs) {
    if (lhs.request_hash != rhs.request_hash ||
        lhs.expected_state_version != rhs.expected_state_version ||
        !(lhs.time == rhs.time) || lhs.actor_id != rhs.actor_id ||
        lhs.controller_person_id != rhs.controller_person_id ||
        lhs.source_account != rhs.source_account ||
        lhs.payments.size() != rhs.payments.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.payments.size(); ++i) {
        if (lhs.payments[i].account_id != rhs.payments[i].account_id ||
            !(lhs.payments[i].amount == rhs.payments[i].amount)) {
            return false;
        }
    }
    return true;
}

bool validRequestHash(const std::string& hash) {
    return hash.size() == 64 && std::all_of(hash.begin(), hash.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

bool follows(const GameTime& next, const GameTime& previous) {
    if (next.day != previous.day) return next.day > previous.day;
    return next.phase >= previous.phase &&
           next.sequence_in_day > previous.sequence_in_day;
}

struct ExecutionGuard {
    bool& executing;
    explicit ExecutionGuard(bool& value) : executing(value) {
        if (executing) throw std::logic_error("settlement execution is not reentrant");
        executing = true;
    }
    ~ExecutionGuard() { executing = false; }
};
} // namespace

struct SettlementService::State {
    struct Record {
        CashTransferCommand command;
        SettlementResult result;
    };
    std::map<AccountId, Money> cash;
    Money initial_total{0};
    std::uint64_t version = 0;
    std::optional<GameTime> last_time;
    std::map<CommandId, Record> processed;

    Money balance(AccountId account) const {
        const auto it = cash.find(account);
        return it == cash.end() ? Money(0) : it->second;
    }

    Money total() const {
        Money sum(0);
        for (const auto& entry : cash) sum = sum.add(entry.second);
        return sum;
    }
};

SettlementService::SettlementService(const ActorRegistry& registry,
                                   const std::map<AccountId, Money>& initial_cash,
                                   SettlementFaultProbe fault_probe)
    : registry_(registry), state_(std::make_unique<State>()),
      fault_probe_(std::move(fault_probe)) {
    for (const auto& entry : initial_cash) {
        registry_.account(entry.first);
        state_->cash.emplace(entry);
    }
    state_->initial_total = state_->total();
}

SettlementService::~SettlementService() = default;

void SettlementService::probe(SettlementStage stage) const {
    if (fault_probe_) fault_probe_(stage);
}

Money SettlementService::balance(AccountId account) const {
    registry_.account(account);
    return state_->balance(account);
}

Money SettlementService::totalCash() const { return state_->total(); }
std::uint64_t SettlementService::stateVersion() const noexcept { return state_->version; }
std::size_t SettlementService::processedCount() const noexcept { return state_->processed.size(); }

std::optional<SettlementResult> SettlementService::result(CommandId id) const {
    const auto it = state_->processed.find(id);
    return it == state_->processed.end() ? std::nullopt
        : std::optional<SettlementResult>(it->second.result);
}

SettlementResult SettlementService::execute(const CashTransferCommand& command) {
    const ExecutionGuard guard(executing_);
    const auto rejected = [&](SettlementReason reason) {
        return SettlementResult{command.command_id, SettlementStatus::Rejected,
                                reason, state_->version, state_->version, Money(0)};
    };
    // Replay precedes state-version/time/registry checks: the original result
    // remains authoritative even if assets or actor activity have since changed.
    const auto previous = state_->processed.find(command.command_id);
    if (previous != state_->processed.end()) {
        return sameRequest(previous->second.command, command)
            ? previous->second.result : rejected(SettlementReason::IdConflict);
    }

    const auto publish = [&](std::unique_ptr<State> candidate,
                             SettlementResult result) {
        candidate->processed.emplace(command.command_id, State::Record{command, result});
        probe(SettlementStage::BeforePublish);
        // All copying, allocation, arithmetic and diagnostics finish first.
        // Swapping the one owner publishes balances, version and result together.
        state_.swap(candidate);
        return result;
    };
    const auto reject = [&](SettlementReason reason) {
        return publish(std::make_unique<State>(*state_), rejected(reason));
    };
    if (!validRequestHash(command.request_hash) || command.payments.empty()) {
        return reject(SettlementReason::InvalidRequest);
    }
    if (command.expected_state_version != state_->version) {
        return reject(SettlementReason::StaleState);
    }
    if (command.time.day == 0 || command.time.sequence_in_day == 0 ||
        (state_->last_time && !follows(command.time, *state_->last_time))) {
        return reject(SettlementReason::InvalidTime);
    }
    if (state_->version == static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return reject(SettlementReason::VersionOverflow);
    }

    // Unknown references are economic rejections; no map insertion can create
    // an unregistered account or imply ownership of another actor's funds.
    const auto validateSource = [&]() {
        try {
            const auto& actor = registry_.actor(command.actor_id);
            const auto& source = registry_.account(command.source_account);
            if (!actor.active) return SettlementReason::InactiveActor;
            const auto controller = actor.kind == ActorKind::Person
                ? std::optional<ActorId>(actor.id) : actor.owner_person_id;
            if (source.owner_actor_id != actor.id || controller != command.controller_person_id) {
                return SettlementReason::Unauthorized;
            }
            if (controller) {
                const auto& person = registry_.actor(*controller);
                if (!person.active || person.kind != ActorKind::Person) {
                    return SettlementReason::Unauthorized;
                }
            }
            return SettlementReason::None;
        } catch (const std::out_of_range&) {
            return SettlementReason::InvalidReference;
        }
    };
    const auto sourceReason = validateSource();
    if (sourceReason != SettlementReason::None) return reject(sourceReason);

    Money total(0);
    for (const auto& payment : command.payments) {
        if (payment.amount.value() == 0 || payment.account_id == command.source_account) {
            return reject(SettlementReason::InvalidRequest);
        }
        try {
            total = total.add(payment.amount);
        } catch (const std::overflow_error&) {
            return reject(SettlementReason::AmountOverflow);
        }
    }
    if (total.value() > state_->balance(command.source_account).value()) {
        return reject(SettlementReason::InsufficientFunds);
    }

    auto candidate = std::make_unique<State>(*state_);
    candidate->cash.insert_or_assign(command.source_account,
        candidate->balance(command.source_account).subtract(total));
    probe(SettlementStage::AfterDebit);
    for (const auto& payment : command.payments) {
        bool active = false;
        try {
            const auto& account = registry_.account(payment.account_id);
            active = registry_.actor(account.owner_actor_id).active;
        } catch (const std::out_of_range&) {
            return reject(SettlementReason::InvalidReference);
        }
        if (!active) return reject(SettlementReason::InactiveActor);
        candidate->cash.insert_or_assign(payment.account_id,
            candidate->balance(payment.account_id).add(payment.amount));
        probe(SettlementStage::AfterCredit);
    }
    if (!(candidate->total() == candidate->initial_total)) {
        throw std::logic_error("settlement violates cash conservation");
    }
    ++candidate->version;
    candidate->last_time.emplace(command.time);
    const SettlementResult result{command.command_id, SettlementStatus::Succeeded,
        SettlementReason::None, state_->version, candidate->version, total};
    return publish(std::move(candidate), result);
}

} // namespace economy
