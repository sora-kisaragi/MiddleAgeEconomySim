#include "core/settlement_service.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>
#include <type_traits>

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

bool sameRequest(const GoodsProcessingCommand& lhs, const GoodsProcessingCommand& rhs) {
    return lhs.request_hash == rhs.request_hash &&
        lhs.expected_state_version == rhs.expected_state_version && lhs.time == rhs.time &&
        lhs.actor_id == rhs.actor_id && lhs.controller_person_id == rhs.controller_person_id &&
        lhs.recipe_id == rhs.recipe_id && lhs.batches == rhs.batches;
}

bool sameRequest(const GoodsFlowCommand& lhs, const GoodsFlowCommand& rhs) {
    return lhs.request_hash == rhs.request_hash &&
        lhs.expected_state_version == rhs.expected_state_version && lhs.time == rhs.time &&
        lhs.actor_id == rhs.actor_id && lhs.controller_person_id == rhs.controller_person_id &&
        lhs.kind == rhs.kind && lhs.product_id == rhs.product_id &&
        lhs.quantity == rhs.quantity && lhs.recipient == rhs.recipient;
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
        std::variant<CashTransferCommand, GoodsProcessingCommand, GoodsFlowCommand> command;
        SettlementResult result;
    };
    std::map<AccountId, Money> cash;
    std::map<std::pair<ActorId, ProductId>, Quantity> goods;
    std::map<ActorId, Quantity> labor;
    std::vector<GoodsEntry> goods_history;
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
                                   SettlementFaultProbe fault_probe, GoodsSetup goods)
    : registry_(registry), state_(std::make_unique<State>()),
      fault_probe_(std::move(fault_probe)), goods_setup_(std::move(goods)) {
    for (const auto& entry : initial_cash) {
        registry_.account(entry.first);
        state_->cash.emplace(entry);
    }
    state_->initial_total = state_->total();
    for (const auto& recipe : goods_setup_.recipes) {
        if (recipe.first.empty() || recipe.second.inputs.empty() || recipe.second.outputs.empty() ||
            recipe.second.labor_per_batch.value() == 0) {
            throw std::invalid_argument("processing recipe needs inputs, outputs and labor");
        }
        for (const auto* lines : {&recipe.second.inputs, &recipe.second.outputs}) {
            for (const auto& line : *lines) {
                if (!goods_setup_.products.count(line.first) || line.second.value() == 0) {
                    throw std::invalid_argument("invalid recipe product or quantity");
                }
            }
        }
    }
    for (const auto& entry : goods_setup_.balances) {
        registry_.actor(entry.first.first);
        if (!goods_setup_.products.count(entry.first.second)) {
            throw std::invalid_argument("unknown opening product");
        }
    }
    for (const auto& entry : goods_setup_.labor) {
        if (registry_.actor(entry.first).kind != ActorKind::Business) {
            throw std::invalid_argument("processing labor belongs to a business");
        }
    }
    state_->goods = goods_setup_.balances;
    state_->labor = goods_setup_.labor;
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

Quantity SettlementService::goodsBalance(ActorId actor, ProductId product) const {
    registry_.actor(actor);
    if (!goods_setup_.products.count(product)) throw std::out_of_range("unknown product");
    const auto it = state_->goods.find({actor, product});
    return it == state_->goods.end() ? Quantity(0) : it->second;
}

Quantity SettlementService::availableLabor(ActorId actor) const {
    registry_.actor(actor);
    const auto it = state_->labor.find(actor);
    return it == state_->labor.end() ? Quantity(0) : it->second;
}

SettlementResult SettlementService::execute(const CashTransferCommand& command) {
    return executeCommand(command);
}

SettlementResult SettlementService::execute(const GoodsProcessingCommand& command) {
    return executeCommand(command);
}

SettlementResult SettlementService::execute(const GoodsFlowCommand& command) {
    return executeCommand(command);
}

std::vector<GoodsEntry> SettlementService::goodsHistory() const {
    return state_->goods_history;
}

template <typename Command>
SettlementResult SettlementService::executeCommand(const Command& command) {
    const ExecutionGuard guard(executing_);
    const auto rejected = [&](SettlementReason reason) {
        return SettlementResult{command.command_id, SettlementStatus::Rejected,
                                reason, state_->version, state_->version, Money(0)};
    };
    // Replay precedes state-version/time/registry checks: the original result
    // remains authoritative even if assets or actor activity have since changed.
    const auto previous = state_->processed.find(command.command_id);
    if (previous != state_->processed.end()) {
        const auto* stored = std::get_if<Command>(&previous->second.command);
        return stored && sameRequest(*stored, command)
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
    if (!validRequestHash(command.request_hash)) {
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
            if constexpr (std::is_same_v<Command, CashTransferCommand>) {
                if (registry_.account(command.source_account).owner_actor_id != actor.id) {
                    return SettlementReason::Unauthorized;
                }
            } else if constexpr (std::is_same_v<Command, GoodsProcessingCommand>) {
                if (actor.kind != ActorKind::Business) return SettlementReason::Unauthorized;
            } else if (command.kind == GoodsFlowKind::Production && actor.kind != ActorKind::Business) {
                return SettlementReason::Unauthorized;
            }
            if (!actor.active) return SettlementReason::InactiveActor;
            const auto controller = actor.kind == ActorKind::Person
                ? std::optional<ActorId>(actor.id) : actor.owner_person_id;
            if (controller != command.controller_person_id) {
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
    auto candidate = std::make_unique<State>(*state_);
    const auto recordGoods = [&](ActorId actor, ProductId product,
                                 GoodsEntryKind kind, Quantity quantity) {
        candidate->goods_history.push_back({command.command_id, command.time,
                                            actor, product, kind, quantity});
    };
    if constexpr (std::is_same_v<Command, GoodsFlowCommand>) {
        if (command.quantity.value() == 0) return reject(SettlementReason::InvalidRequest);
        if (!goods_setup_.products.count(command.product_id)) {
            return reject(SettlementReason::InvalidReference);
        }
        switch (command.kind) {
        case GoodsFlowKind::Production:
        case GoodsFlowKind::Consumption:
        case GoodsFlowKind::Disposal:
            if (command.recipient) return reject(SettlementReason::InvalidRequest);
            break;
        case GoodsFlowKind::Transfer:
            if (!command.recipient || *command.recipient == command.actor_id) {
                return reject(SettlementReason::InvalidRequest);
            }
            try {
                if (!registry_.actor(*command.recipient).active) {
                    return reject(SettlementReason::InactiveActor);
                }
            } catch (const std::out_of_range&) {
                return reject(SettlementReason::InvalidReference);
            }
            break;
        default:
            return reject(SettlementReason::InvalidRequest);
        }
        const auto before = goodsBalance(command.actor_id, command.product_id);
        const auto key = std::make_pair(command.actor_id, command.product_id);
        if (command.kind == GoodsFlowKind::Production) {
            try {
                candidate->goods.insert_or_assign(key, before.add(command.quantity));
            } catch (const std::overflow_error&) {
                return reject(SettlementReason::AmountOverflow);
            }
            recordGoods(command.actor_id, command.product_id, GoodsEntryKind::Production, command.quantity);
            probe(SettlementStage::AfterOutput);
        } else {
            if (before.value() < command.quantity.value()) return reject(SettlementReason::InsufficientGoods);
            candidate->goods.insert_or_assign(key, before.subtract(command.quantity));
            const auto kind = command.kind == GoodsFlowKind::Transfer ? GoodsEntryKind::Delivery
                : command.kind == GoodsFlowKind::Consumption ? GoodsEntryKind::Consumption : GoodsEntryKind::Disposal;
            recordGoods(command.actor_id, command.product_id, kind, command.quantity);
            probe(SettlementStage::AfterInput);
            if (command.kind == GoodsFlowKind::Transfer) {
                const auto recipient = *command.recipient;
                try {
                    candidate->goods.insert_or_assign({recipient, command.product_id},
                        goodsBalance(recipient, command.product_id).add(command.quantity));
                } catch (const std::overflow_error&) {
                    return reject(SettlementReason::AmountOverflow);
                }
                recordGoods(recipient, command.product_id, GoodsEntryKind::Receipt, command.quantity);
                probe(SettlementStage::AfterOutput);
            }
        }
    } else if constexpr (std::is_same_v<Command, GoodsProcessingCommand>) {
        if (command.time.phase != 6 || command.batches.value() == 0) {
            return reject(SettlementReason::InvalidRequest);
        }
        const auto recipe = goods_setup_.recipes.find(command.recipe_id);
        if (recipe == goods_setup_.recipes.end()) return reject(SettlementReason::InvalidReference);
        Quantity labor(0);
        std::map<ProductId, Quantity> inputs, outputs;
        try {
            labor = recipe->second.labor_per_batch.multiply(command.batches.value());
            for (const auto& line : recipe->second.inputs) {
                inputs.emplace(line.first, line.second.multiply(command.batches.value()));
            }
            for (const auto& line : recipe->second.outputs) {
                outputs.emplace(line.first, line.second.multiply(command.batches.value()));
            }
        } catch (const std::overflow_error&) {
            return reject(SettlementReason::AmountOverflow);
        }
        if (labor.value() > availableLabor(command.actor_id).value()) {
            return reject(SettlementReason::InsufficientLabor);
        }
        candidate->labor.insert_or_assign(command.actor_id,
            availableLabor(command.actor_id).subtract(labor));
        for (const auto& input : inputs) {
            const auto before = goodsBalance(command.actor_id, input.first);
            if (input.second.value() > before.value()) return reject(SettlementReason::InsufficientGoods);
            candidate->goods.insert_or_assign({command.actor_id, input.first}, before.subtract(input.second));
            recordGoods(command.actor_id, input.first, GoodsEntryKind::Input, input.second);
            probe(SettlementStage::AfterInput);
        }
        for (const auto& output : outputs) {
            const auto key = std::make_pair(command.actor_id, output.first);
            const auto it = candidate->goods.find(key);
            const auto before = it == candidate->goods.end() ? Quantity(0) : it->second;
            Quantity after(0);
            try {
                after = before.add(output.second);
            } catch (const std::overflow_error&) {
                return reject(SettlementReason::AmountOverflow);
            }
            candidate->goods.insert_or_assign(key, after);
            recordGoods(command.actor_id, output.first, GoodsEntryKind::Production, output.second);
            probe(SettlementStage::AfterOutput);
        }
    } else {
        if (command.payments.empty()) return reject(SettlementReason::InvalidRequest);
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
