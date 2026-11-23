#pragma once

#include "core/actor_registry.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace economy {

struct CashPayment {
    AccountId account_id;
    Money amount;
};

// Identity and request_hash are supplied by the trusted simulation driver.
// Full payload equality is also checked, so reusing a hash cannot hide changes.
struct CashTransferCommand {
    CommandId command_id;
    std::string request_hash;
    std::uint64_t expected_state_version;
    GameTime time;
    ActorId actor_id;
    std::optional<ActorId> controller_person_id;
    AccountId source_account;
    std::vector<CashPayment> payments;
};

struct ProcessingRecipe {
    std::map<ProductId, Quantity> inputs;
    std::map<ProductId, Quantity> outputs;
    Quantity labor_per_batch{0};
};

// Trusted, immutable configuration and opening inventory/labor for this
// development path. It is not a mirror of legacy Business.stock.
struct GoodsSetup {
    std::set<ProductId> products;
    std::map<std::string, ProcessingRecipe> recipes;
    std::map<std::pair<ActorId, ProductId>, Quantity> balances;
    std::map<ActorId, Quantity> labor;
};

struct GoodsProcessingCommand {
    CommandId command_id;
    std::string request_hash;
    std::uint64_t expected_state_version;
    GameTime time;
    ActorId actor_id;
    std::optional<ActorId> controller_person_id;
    std::string recipe_id;
    Quantity batches;
};

enum class SettlementStatus { Succeeded, PartiallySucceeded, Rejected };
enum class SettlementReason {
    None, IdConflict, InvalidRequest, InvalidTime, StaleState,
    InvalidReference, Unauthorized, InactiveActor, InsufficientFunds,
    AmountOverflow, VersionOverflow, InsufficientGoods, InsufficientLabor
};

struct SettlementResult {
    CommandId command_id;
    SettlementStatus status;
    SettlementReason reason;
    std::uint64_t version_before;
    std::uint64_t version_after;
    Money amount;
};

// Test-only fault injection at candidate mutation and publication boundaries.
// A probe must not mutate the registry or call execute recursively.
enum class SettlementStage { AfterDebit, AfterCredit, BeforePublish, AfterInput, AfterOutput };
using SettlementFaultProbe = std::function<void(SettlementStage)>;

// Single-writer, in-memory command boundary. Registry lifetime must exceed the
// service's; a trusted driver owns both and can add/close actors between calls.
// Cash here is the sole balance for this new path, not a mirror of Agent.money.
class SettlementService {
public:
    SettlementService(const ActorRegistry& registry,
                      const std::map<AccountId, Money>& initial_cash,
                      SettlementFaultProbe fault_probe = {},
                      GoodsSetup goods = {});
    ~SettlementService();

    // Economic rejections are saved once. Calculation/allocation exceptions
    // propagate without publishing assets or the command result.
    SettlementResult execute(const CashTransferCommand& command);
    SettlementResult execute(const GoodsProcessingCommand& command);

    Quantity goodsBalance(ActorId actor, ProductId product) const;
    Quantity availableLabor(ActorId actor) const;

    Money balance(AccountId account) const;
    Money totalCash() const;
    std::uint64_t stateVersion() const noexcept;
    std::size_t processedCount() const noexcept;
    std::optional<SettlementResult> result(CommandId id) const;

private:
    struct State;
    const ActorRegistry& registry_;
    std::unique_ptr<State> state_;
    SettlementFaultProbe fault_probe_;
    const GoodsSetup goods_setup_;
    bool executing_ = false;

    void probe(SettlementStage stage) const;
    template <typename Command>
    SettlementResult executeCommand(const Command& command);
};

} // namespace economy
