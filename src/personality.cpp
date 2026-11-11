#include "agent/personality.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace economy {
namespace {
constexpr std::array<std::uint32_t, 64> round_constants = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::uint32_t rotateRight(std::uint32_t value, unsigned int bits) {
    return (value >> bits) | (value << (32 - bits));
}

void appendLengthPrefixed(std::vector<std::uint8_t>& output, const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("personality field exceeds uint32 length");
    }
    const auto length = static_cast<std::uint32_t>(value.size());
    for (int shift = 24; shift >= 0; shift -= 8) {
        output.push_back(static_cast<std::uint8_t>(length >> shift));
    }
    output.insert(output.end(), value.begin(), value.end());
}
} // namespace

std::array<std::uint8_t, 32> sha256(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() > std::numeric_limits<std::uint64_t>::max() / 8) {
        throw std::length_error("SHA-256 input bit length overflow");
    }
    std::vector<std::uint8_t> padded = bytes;
    padded.push_back(0x80);
    while (padded.size() % 64 != 56) padded.push_back(0);
    const auto bit_length = static_cast<std::uint64_t>(bytes.size()) * 8;
    for (int shift = 56; shift >= 0; shift -= 8) {
        padded.push_back(static_cast<std::uint8_t>(bit_length >> shift));
    }

    std::array<std::uint32_t, 8> hash = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    for (std::size_t offset = 0; offset < padded.size(); offset += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            for (std::size_t j = 0; j < 4; ++j) {
                words[i] = (words[i] << 8) | padded[offset + i * 4 + j];
            }
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = rotateRight(words[i - 15], 7) ^ rotateRight(words[i - 15], 18) ^ (words[i - 15] >> 3);
            const auto s1 = rotateRight(words[i - 2], 17) ^ rotateRight(words[i - 2], 19) ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        auto a = hash[0], b = hash[1], c = hash[2], d = hash[3];
        auto e = hash[4], f = hash[5], g = hash[6], h = hash[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const auto choice = (e & f) ^ (~e & g);
            const auto temp1 = h + s1 + choice + round_constants[i] + words[i];
            const auto s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = s0 + majority;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d;
        hash[4] += e; hash[5] += f; hash[6] += g; hash[7] += h;
    }
    std::array<std::uint8_t, 32> digest{};
    for (std::size_t i = 0; i < hash.size(); ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            digest[i * 4 + j] = static_cast<std::uint8_t>(hash[i] >> (24 - j * 8));
        }
    }
    return digest;
}

const std::array<std::string, 7>& traitKeys() {
    static const std::array<std::string, 7> keys = {
        "bread_preference", "cash_caution", "debt_tolerance", "job_loyalty",
        "margin_preference", "inventory_preference", "price_flexibility"};
    return keys;
}

Trait traitFromKey(const std::string& key) {
    const auto& keys = traitKeys();
    const auto it = std::find(keys.begin(), keys.end(), key);
    if (it == keys.end()) throw std::invalid_argument("unknown personality trait");
    return static_cast<Trait>(it - keys.begin());
}

std::uint16_t Personality::get(Trait trait) const {
    const auto index = static_cast<std::size_t>(trait);
    if (index >= values_.size()) throw std::invalid_argument("unknown personality trait");
    return values_[index];
}

Personality::Personality(std::array<std::uint16_t, 7> values) : values_(values) {
    for (const auto value : values_) {
        if (value > 10000) throw std::invalid_argument("personality trait out of range");
    }
}

Personality generatePersonality(std::uint64_t master_seed, ActorId person_id) {
    std::array<std::uint16_t, 7> values{};
    constexpr std::uint64_t modulus = 10001;
    constexpr std::uint64_t limit = (std::uint64_t{1} << 32) / modulus * modulus;
    for (std::size_t index = 0; index < values.size(); ++index) {
        std::uint64_t draw_index = 0;
        for (;;) {
            std::vector<std::uint8_t> input;
            appendLengthPrefixed(input, "personality-v1");
            appendLengthPrefixed(input, std::to_string(master_seed));
            appendLengthPrefixed(input, std::to_string(person_id.value()));
            appendLengthPrefixed(input, traitKeys()[index]);
            appendLengthPrefixed(input, std::to_string(draw_index));
            const auto hash = sha256(input);
            const std::uint64_t value = (std::uint64_t(hash[0]) << 24) |
                (std::uint64_t(hash[1]) << 16) | (std::uint64_t(hash[2]) << 8) | hash[3];
            if (value < limit) {
                values[index] = static_cast<std::uint16_t>(value % modulus);
                break;
            }
            if (draw_index == std::numeric_limits<std::uint64_t>::max()) {
                throw std::overflow_error("personality draw index overflow");
            }
            ++draw_index;
        }
    }
    return Personality(values);
}

std::int64_t traitCoefficient(std::int64_t lo, std::int64_t hi, std::uint16_t trait) {
    if (lo < 0 || hi < lo || trait > 10000) {
        throw std::invalid_argument("invalid trait coefficient range");
    }
    return checkedAdd(lo, checkedMultiply(hi - lo, trait) / 10000);
}

} // namespace economy
