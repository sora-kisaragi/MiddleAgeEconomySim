#pragma once

#include "core/economy_types.h"
#include "agent/personality.h"

#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace economy {

enum class ActorKind { Person, Business, Lender, Government };

struct ActorSpec {
    std::string key;
    std::string name;
    ActorKind kind;
    std::string owner_key; // Required for businesses, empty otherwise.
    bool retail_store;

    ActorSpec(std::string key, std::string name, ActorKind kind,
              std::string owner_key = {}, bool retail_store = false)
        : key(std::move(key)), name(std::move(name)), kind(kind),
          owner_key(std::move(owner_key)), retail_store(retail_store) {}
};

struct ActorRecord {
    ActorId id;
    AccountId account_id;
    std::string key;
    std::string name;
    ActorKind kind;
    std::optional<ActorId> owner_person_id;
    std::optional<Personality> personality;
    bool active;
};

struct CashAccountRecord {
    AccountId id;
    ActorId owner_actor_id;
};

struct StoreRecord {
    StoreId id;
    ActorId operator_business_id;
    bool active;
};

// Registry metadata is retained after a business stops operating. Economic
// balances live in the separate settlement ledger (#44), not in this registry.
class ActorRegistry {
public:
    static ActorRegistry fromSpecs(const std::vector<ActorSpec>& specs, std::uint64_t master_seed = 0) {
        std::vector<std::string> keys;
        std::vector<std::string> store_keys;
        std::map<std::string, ActorSpec> by_key;
        int lenders = 0;
        int governments = 0;
        for (const auto& spec : specs) {
            if (spec.key.empty() || spec.name.empty() || !by_key.emplace(spec.key, spec).second) {
                throw std::invalid_argument("actor key and name must be nonempty and keys unique");
            }
            switch (spec.kind) {
                case ActorKind::Person: break;
                case ActorKind::Business: break;
                case ActorKind::Lender: ++lenders; break;
                case ActorKind::Government: ++governments; break;
                default: throw std::invalid_argument("invalid actor kind");
            }
            if (spec.retail_store) {
                if (spec.kind != ActorKind::Business) {
                    throw std::invalid_argument("only a business can operate a store");
                }
                store_keys.push_back(spec.key);
            }
            keys.push_back(spec.key);
        }
        if (lenders != 1 || governments != 1) {
            throw std::invalid_argument("village requires one lender and one government");
        }
        const auto actor_ids = assignStableIds<ActorTag>(keys);
        const auto account_ids = assignStableIds<AccountTag>(keys);
        const auto store_ids = assignStableIds<StoreTag>(store_keys);
        ActorRegistry registry;
        for (const auto& pair : by_key) {
            const auto& spec = pair.second;
            std::optional<ActorId> owner;
            if (spec.kind == ActorKind::Business) {
                const auto it = by_key.find(spec.owner_key);
                if (it == by_key.end() || it->second.kind != ActorKind::Person) {
                    throw std::invalid_argument("business owner must refer to a person");
                }
                owner = actor_ids.at(spec.owner_key);
            } else if (!spec.owner_key.empty()) {
                throw std::invalid_argument("non-business actor cannot have a business owner");
            }
            const ActorId actor_id = actor_ids.at(spec.key);
            const AccountId account_id = account_ids.at(spec.key);
            const std::optional<Personality> personality = spec.kind == ActorKind::Person
                ? std::optional<Personality>(generatePersonality(master_seed, actor_id)) : std::nullopt;
            registry.actors_.emplace(actor_id, ActorRecord{actor_id, account_id, spec.key,
                                                             spec.name, spec.kind, owner, personality, true});
            registry.accounts_.emplace(account_id, CashAccountRecord{account_id, actor_id});
            registry.keys_.emplace(spec.key, actor_id);
            if (spec.kind == ActorKind::Person) ++registry.population_;
            if (spec.retail_store) {
                const StoreId store_id = store_ids.at(spec.key);
                registry.stores_.emplace(store_id, StoreRecord{store_id, actor_id, true});
                registry.business_stores_.emplace(actor_id, store_id);
            }
        }
        registry.next_actor_ = actor_ids.size() + 1;
        registry.next_account_ = account_ids.size() + 1;
        registry.next_store_ = store_ids.size() + 1;
        return registry;
    }

    const ActorRecord& actor(ActorId id) const { return actors_.at(id); }
    const ActorRecord& actorByKey(const std::string& key) const { return actor(keys_.at(key)); }
    const CashAccountRecord& account(AccountId id) const { return accounts_.at(id); }
    const CashAccountRecord& accountFor(ActorId actor_id) const {
        return account(actor(actor_id).account_id);
    }
    const StoreRecord& store(StoreId id) const { return stores_.at(id); }
    StoreId storeFor(ActorId business_id) const { return business_stores_.at(business_id); }
    std::size_t population() const noexcept { return population_; }
    std::size_t actorCount() const noexcept { return actors_.size(); }

    ActorId addBusiness(const std::string& key, const std::string& name, ActorId owner_id,
                        bool retail_store) {
        const auto owner = actors_.find(owner_id);
        if (key.empty() || name.empty() || keys_.count(key) != 0 || owner == actors_.end() ||
            owner->second.kind != ActorKind::Person || !owner->second.active) {
            throw std::invalid_argument("invalid new business or owner");
        }
        // Validate all generated IDs before changing any registry collection.
        const ActorId actor_id(next_actor_);
        const AccountId account_id(next_account_);
        const std::optional<StoreId> store_id = retail_store
            ? std::optional<StoreId>(StoreId(next_store_)) : std::nullopt;
        try {
            actors_.emplace(actor_id, ActorRecord{actor_id, account_id, key, name,
                                                  ActorKind::Business, owner_id, std::nullopt, true});
            accounts_.emplace(account_id, CashAccountRecord{account_id, actor_id});
            keys_.emplace(key, actor_id);
            if (store_id) {
                stores_.emplace(*store_id, StoreRecord{*store_id, actor_id, true});
                business_stores_.emplace(actor_id, *store_id);
            }
        } catch (...) {
            business_stores_.erase(actor_id);
            if (store_id) stores_.erase(*store_id);
            keys_.erase(key);
            accounts_.erase(account_id);
            actors_.erase(actor_id);
            throw;
        }
        if (store_id) ++next_store_;
        ++next_actor_;
        ++next_account_;
        return actor_id;
    }

    void closeBusiness(ActorId business_id) {
        auto& business = actors_.at(business_id);
        if (business.kind != ActorKind::Business) {
            throw std::invalid_argument("only a business can close");
        }
        business.active = false;
        auto store_it = business_stores_.find(business_id);
        if (store_it != business_stores_.end()) stores_.at(store_it->second).active = false;
    }

private:
    ActorRegistry() = default;
    std::map<ActorId, ActorRecord> actors_;
    std::map<AccountId, CashAccountRecord> accounts_;
    std::map<StoreId, StoreRecord> stores_;
    std::map<std::string, ActorId> keys_;
    std::map<ActorId, StoreId> business_stores_;
    std::size_t population_ = 0;
    std::uint64_t next_actor_ = 1;
    std::uint64_t next_account_ = 1;
    std::uint64_t next_store_ = 1;
};

} // namespace economy
