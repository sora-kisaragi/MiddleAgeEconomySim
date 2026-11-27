#pragma once
#include "core/settlement_service.h"

// Runtime ownership adapter. Initial legacy fields are imported once; after
// binding they are not read as balances or synchronized as a second ledger.
class RuntimeGoods {
public:
    RuntimeGoods(std::shared_ptr<const economy::ActorRegistry> registry, economy::GoodsSetup setup,
                 std::map<std::string, economy::ProductId> products)
        : registry_owner_(std::move(registry)), registry_(requireRegistry(registry_owner_)), products_(std::move(products)),
          service_(registry_, {}, {}, setup), opening_(std::move(setup.balances)) { validateProducts(setup.products); }
    RuntimeGoods(const economy::ActorRegistry& registry, economy::GoodsSetup setup,
                 std::map<std::string, economy::ProductId> products)
        : registry_(registry), products_(std::move(products)),
          service_(registry, {}, {}, setup), opening_(std::move(setup.balances)) { validateProducts(setup.products); }
    bool isActiveBusiness(economy::ActorId actor) const {
        const auto& record = registry_.actor(actor);
        return record.active && record.kind == economy::ActorKind::Business;
    }
    void beginDay() { ++day_; sequence_ = 0; }
    std::int64_t balance(economy::ActorId actor, const std::string& product) const {
        return service_.goodsBalance(actor, products_.at(product)).value();
    }
    bool move(economy::ActorId actor, const std::string& product, std::int64_t amount,
              economy::GoodsFlowKind kind, std::optional<economy::ActorId> to = {}) {
        const auto& record = registry_.actor(actor);
        const auto controller = record.kind == economy::ActorKind::Person
            ? std::optional<economy::ActorId>(actor) : record.owner_person_id;
        const auto sequence = ++sequence_;
        const economy::GoodsFlowCommand command{economy::CommandId(++command_),
            std::string(64, 'a'), service_.stateVersion(), economy::GameTime(day_, 6, sequence),
            actor, controller, kind, products_.at(product), economy::Quantity(amount), to};
        return service_.execute(command).status == economy::SettlementStatus::Succeeded;
    }
    bool process(economy::ActorId actor, const std::string& recipe, std::int64_t batches) {
        const auto& record = registry_.actor(actor);
        const economy::GoodsProcessingCommand command{economy::CommandId(++command_),
            std::string(64, 'a'), service_.stateVersion(), economy::GameTime(day_, 6, ++sequence_),
            actor, record.owner_person_id, recipe, economy::Quantity(batches)};
        return service_.execute(command).status == economy::SettlementStatus::Succeeded;
    }
    bool audit() const {
        std::map<std::pair<economy::ActorId, economy::ProductId>, std::int64_t> balances;
        for (const auto& row : opening_) balances.emplace(row.first, row.second.value());
        for (const auto& entry : service_.goodsHistory()) {
            auto& amount = balances[{entry.actor_id, entry.product_id}];
            const bool incoming = entry.kind == economy::GoodsEntryKind::Production ||
                                  entry.kind == economy::GoodsEntryKind::Receipt;
            if (incoming) {
                if (entry.quantity.value() > std::numeric_limits<std::int64_t>::max() - amount) return false;
                amount += entry.quantity.value();
            } else {
                if (entry.quantity.value() > amount) return false;
                amount -= entry.quantity.value();
            }
        }
        for (const auto& row : balances) {
            if (service_.goodsBalance(row.first.first, row.first.second).value() != row.second) return false;
        }
        return true;
    }
    std::vector<economy::GoodsEntry> history() const { return service_.goodsHistory(); }
private:
    void validateProducts(const std::set<economy::ProductId>& catalog) const {
        std::set<economy::ProductId> used;
        for (const auto& row : products_) {
            if (row.first.empty() || !catalog.count(row.second) || !used.insert(row.second).second)
                throw std::invalid_argument("product names must uniquely cover catalog IDs");
        }
        if (used != catalog) throw std::invalid_argument("product catalog name missing");
    }
    static const economy::ActorRegistry& requireRegistry(
            const std::shared_ptr<const economy::ActorRegistry>& registry) {
        if (!registry) throw std::invalid_argument("goods registry must not be null");
        return *registry;
    }
    std::shared_ptr<const economy::ActorRegistry> registry_owner_;
    const economy::ActorRegistry& registry_;
    std::map<std::string, economy::ProductId> products_;
    economy::SettlementService service_;
    const std::map<std::pair<economy::ActorId, economy::ProductId>, economy::Quantity> opening_;
    std::uint64_t day_ = 0, sequence_ = 0, command_ = 0;
};
