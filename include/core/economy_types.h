#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace economy {

// IDs are scoped to a single run. Zero is reserved for an absent reference.
template <typename Tag>
class Id {
public:
    explicit Id(std::uint64_t value) : value_(value) {
        if (value == 0 || value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            throw std::invalid_argument("ID must be between 1 and INT64_MAX");
        }
    }

    std::uint64_t value() const noexcept { return value_; }
    friend bool operator==(Id lhs, Id rhs) noexcept { return lhs.value_ == rhs.value_; }
    friend bool operator!=(Id lhs, Id rhs) noexcept { return !(lhs == rhs); }
    friend bool operator<(Id lhs, Id rhs) noexcept { return lhs.value_ < rhs.value_; }

private:
    std::uint64_t value_;
};

struct ActorTag {};
struct AccountTag {};
struct StoreTag {};
struct ProductTag {};
struct ToolTag {};
struct LoanTag {};
struct IntentTag {};
struct OrderTag {};
struct CommandTag {};
struct EventTag {};

using ActorId = Id<ActorTag>;
using AccountId = Id<AccountTag>;
using StoreId = Id<StoreTag>;
using ProductId = Id<ProductTag>;
using ToolId = Id<ToolTag>;
using LoanId = Id<LoanTag>;
using IntentId = Id<IntentTag>;
using OrderId = Id<OrderTag>;
using CommandId = Id<CommandTag>;
using EventId = Id<EventTag>;

// Sort stable configuration keys before assigning IDs; input traversal order is irrelevant.
template <typename Tag>
std::map<std::string, Id<Tag>> assignStableIds(std::vector<std::string> keys) {
    std::sort(keys.begin(), keys.end());
    std::map<std::string, Id<Tag>> ids;
    for (const auto& key : keys) {
        if (key.empty() || ids.find(key) != ids.end()) {
            throw std::invalid_argument("configuration keys must be nonempty and unique");
        }
        ids.emplace(key, Id<Tag>(ids.size() + 1));
    }
    return ids;
}

inline std::int64_t checkedAdd(std::int64_t lhs, std::int64_t rhs) {
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    if (lhs < 0 || rhs < 0) {
        throw std::invalid_argument("nonnegative operands required");
    }
    if (rhs > max - lhs) {
        throw std::overflow_error("integer addition overflow");
    }
    return lhs + rhs;
}

inline std::int64_t checkedMultiply(std::int64_t lhs, std::int64_t rhs) {
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    if (lhs < 0 || rhs < 0) {
        throw std::invalid_argument("nonnegative operands required");
    }
    if (rhs != 0 && lhs > max / rhs) {
        throw std::overflow_error("integer multiplication overflow");
    }
    return lhs * rhs;
}

template <typename Tag>
class NonnegativeAmount {
public:
    explicit NonnegativeAmount(std::int64_t value) : value_(value) {
        if (value < 0) {
            throw std::invalid_argument("amount must be nonnegative");
        }
    }

    std::int64_t value() const noexcept { return value_; }
    NonnegativeAmount add(NonnegativeAmount other) const {
        return NonnegativeAmount(checkedAdd(value_, other.value_));
    }
    NonnegativeAmount subtract(NonnegativeAmount other) const {
        if (other.value_ > value_) {
            throw std::underflow_error("amount subtraction underflow");
        }
        return NonnegativeAmount(value_ - other.value_);
    }
    NonnegativeAmount multiply(std::int64_t multiplier) const {
        return NonnegativeAmount(checkedMultiply(value_, multiplier));
    }
    friend bool operator==(NonnegativeAmount lhs, NonnegativeAmount rhs) noexcept {
        return lhs.value_ == rhs.value_;
    }

private:
    std::int64_t value_;
};

struct MoneyTag {};
struct QuantityTag {};
using Money = NonnegativeAmount<MoneyTag>;
using Quantity = NonnegativeAmount<QuantityTag>;

// A rational rate is evaluated using checked integer multiplication before division.
struct Rate {
    const std::int64_t numerator;
    const std::int64_t denominator;

    Rate(std::int64_t numerator, std::int64_t denominator)
        : numerator(numerator), denominator(denominator) {
        if (numerator < 0 || denominator <= 0) {
            throw std::invalid_argument("rate requires a nonnegative numerator and positive denominator");
        }
    }

    Money applyFloor(Money amount) const {
        return Money(checkedMultiply(amount.value(), numerator) / denominator);
    }
};

// Day 0 is initialization. Simulated days have phases 1 through 12.
struct GameTime {
    const std::uint64_t day;
    const std::uint8_t phase;
    const std::uint64_t sequence_in_day;

    GameTime(std::uint64_t day, unsigned int phase, std::uint64_t sequence_in_day)
        : day(day), phase(static_cast<std::uint8_t>(phase)), sequence_in_day(sequence_in_day) {
        if (day > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
            (day == 0 ? phase != 0 : phase < 1 || phase > 12) ||
            sequence_in_day > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            throw std::invalid_argument("invalid game time");
        }
    }

    friend bool operator==(const GameTime& lhs, const GameTime& rhs) noexcept {
        return lhs.day == rhs.day && lhs.phase == rhs.phase && lhs.sequence_in_day == rhs.sequence_in_day;
    }
    friend bool operator<(const GameTime& lhs, const GameTime& rhs) noexcept {
        if (lhs.day != rhs.day) return lhs.day < rhs.day;
        if (lhs.phase != rhs.phase) return lhs.phase < rhs.phase;
        return lhs.sequence_in_day < rhs.sequence_in_day;
    }
};

// Called in the simulation's fixed processing order. Sequence numbers are
// unique within a day, including across phase boundaries; a new day resets
// the counter. A checkpoint can restore the last committed time.
class GameTimeSequencer {
public:
    GameTimeSequencer() = default;
    explicit GameTimeSequencer(GameTime last_committed) : last_(last_committed) {}

    GameTime next(std::uint64_t day, unsigned int phase) {
        // Validate the requested time before advancing the stored position.
        GameTime(day, phase, 0);
        if (last_ && (day < last_->day ||
                      (day == last_->day && phase < last_->phase))) {
            throw std::invalid_argument("game time cannot move backwards");
        }
        if (day == 0) throw std::invalid_argument("day zero is initialization only");
        const std::uint64_t sequence = last_ && day == last_->day
            ? last_->sequence_in_day + 1 : 1;
        const GameTime candidate(day, phase, sequence);
        last_.emplace(candidate);
        return candidate;
    }

    const std::optional<GameTime>& last() const noexcept { return last_; }

private:
    std::optional<GameTime> last_;
};

} // namespace economy
