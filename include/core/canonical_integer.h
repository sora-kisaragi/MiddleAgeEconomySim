#pragma once

#include "core/economy_types.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace economy {

class SignedInteger {
public:
    explicit SignedInteger(std::int64_t value) : value_(value) {}
    std::int64_t value() const noexcept { return value_; }

private:
    std::int64_t value_;
};

namespace detail {
inline std::uint64_t parseMagnitude(const std::string& value, std::size_t first,
                                    std::uint64_t limit) {
    if (first >= value.size() || (value[first] == '0' && first + 1 != value.size())) {
        throw std::invalid_argument("noncanonical decimal integer");
    }
    std::uint64_t number = 0;
    for (std::size_t i = first; i < value.size(); ++i) {
        const char c = value[i];
        if (c < '0' || c > '9') throw std::invalid_argument("noncanonical decimal integer");
        const auto digit = static_cast<std::uint64_t>(c - '0');
        if (digit > limit || number > (limit - digit) / 10) {
            throw std::out_of_range("decimal integer exceeds limit");
        }
        number = number * 10 + digit;
    }
    return number;
}
} // namespace detail

inline std::int64_t parseCanonicalNonnegative(const std::string& value) {
    return static_cast<std::int64_t>(detail::parseMagnitude(
        value, 0, static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())));
}

inline std::int64_t parseCanonicalPositive(const std::string& value) {
    const auto result = parseCanonicalNonnegative(value);
    if (result == 0) throw std::invalid_argument("ID must be positive");
    return result;
}

inline SignedInteger parseCanonicalSigned(const std::string& value) {
    if (!value.empty() && value.front() == '-') {
        constexpr auto limit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1;
        const auto magnitude = detail::parseMagnitude(value, 1, limit);
        if (magnitude == 0) throw std::invalid_argument("negative zero is not canonical");
        if (magnitude == limit) return SignedInteger(std::numeric_limits<std::int64_t>::min());
        return SignedInteger(-static_cast<std::int64_t>(magnitude));
    }
    return SignedInteger(parseCanonicalNonnegative(value));
}

template <typename Tag>
Id<Tag> parseCanonicalId(const std::string& value) {
    return Id<Tag>(static_cast<std::uint64_t>(parseCanonicalPositive(value)));
}

inline Money parseCanonicalMoney(const std::string& value) {
    return Money(parseCanonicalNonnegative(value));
}

inline Quantity parseCanonicalQuantity(const std::string& value) {
    return Quantity(parseCanonicalNonnegative(value));
}

template <typename Tag>
std::string canonicalDecimal(Id<Tag> id) { return std::to_string(id.value()); }

template <typename Tag>
std::string canonicalDecimal(NonnegativeAmount<Tag> value) { return std::to_string(value.value()); }

inline std::string canonicalDecimal(SignedInteger value) { return std::to_string(value.value()); }

} // namespace economy
