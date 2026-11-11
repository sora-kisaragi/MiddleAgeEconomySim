#pragma once

#include "core/economy_types.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace economy {

enum class Trait : std::size_t {
    BreadPreference, CashCaution, DebtTolerance, JobLoyalty,
    MarginPreference, InventoryPreference, PriceFlexibility
};

const std::array<std::string, 7>& traitKeys();
Trait traitFromKey(const std::string& key);

class Personality {
public:
    explicit Personality(std::array<std::uint16_t, 7> values);
    std::uint16_t get(Trait trait) const;
    const std::array<std::uint16_t, 7>& values() const noexcept { return values_; }
    bool operator==(const Personality& other) const noexcept { return values_ == other.values_; }

private:
    const std::array<std::uint16_t, 7> values_;
};

// Exposed for the fixed SHA-256 vectors; not intended for authentication.
std::array<std::uint8_t, 32> sha256(const std::vector<std::uint8_t>& bytes);

Personality generatePersonality(std::uint64_t master_seed, ActorId person_id);
std::int64_t traitCoefficient(std::int64_t lo, std::int64_t hi, std::uint16_t trait);

} // namespace economy
