#include "agent/personality.h"
#include "core/actor_registry.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace economy;

namespace {
std::string hex(const std::array<std::uint8_t, 32>& digest) {
    std::ostringstream out;
    for (const auto byte : digest) {
        out << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
    }
    return out.str();
}
} // namespace

TEST(PersonalityTest, Sha256KnownVectors) {
    EXPECT_EQ(hex(sha256({})), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(hex(sha256({'a', 'b', 'c'})),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(PersonalityTest, DesignVectorForSeed42AndPerson17) {
    const auto traits = generatePersonality(42, ActorId(17));
    EXPECT_EQ(traits.values(), (std::array<std::uint16_t, 7>{8922, 5419, 1163, 6782, 3688, 7545, 8481}));
}

TEST(PersonalityTest, SamePersonIsIndependentOfEvaluationOrder) {
    const auto initial = generatePersonality(42, ActorId(17));
    const auto other = generatePersonality(42, ActorId(18));
    EXPECT_EQ(generatePersonality(42, ActorId(17)), initial);
    EXPECT_NE(other.values(), initial.values());
    EXPECT_NE(generatePersonality(43, ActorId(17)).values(), initial.values());
    EXPECT_EQ(initial.get(traitFromKey("bread_preference")), 8922);
    EXPECT_THROW(traitFromKey("unknown"), std::invalid_argument);
    EXPECT_THROW(initial.get(static_cast<Trait>(99)), std::invalid_argument);
    EXPECT_THROW(generatePersonality(42, ActorId(0)), std::invalid_argument);
    EXPECT_THROW((Personality({0, 0, 0, 0, 0, 0, 10001})), std::invalid_argument);
}

TEST(PersonalityTest, RegistrationGeneratesOnlyPersonalityForPeople) {
    std::vector<ActorSpec> specs = {
        {"person:p17", "商人", ActorKind::Person},
        {"business:b4", "店舗", ActorKind::Business, "person:p17", true},
        {"institution:lender", "金貸", ActorKind::Lender},
        {"institution:government", "政府", ActorKind::Government}};
    const auto first = ActorRegistry::fromSpecs(specs, 42);
    std::reverse(specs.begin(), specs.end());
    const auto second = ActorRegistry::fromSpecs(specs, 42);
    const auto& person = first.actorByKey("person:p17");
    ASSERT_TRUE(person.personality.has_value());
    EXPECT_EQ(person.personality.value(), generatePersonality(42, person.id));
    EXPECT_EQ(person.personality, second.actorByKey("person:p17").personality);
    EXPECT_FALSE(first.actorByKey("business:b4").personality.has_value());
}

TEST(PersonalityTest, CoefficientUsesCheckedIntegerArithmetic) {
    EXPECT_EQ(traitCoefficient(2, 12, 5000), 7);
    EXPECT_EQ(traitCoefficient(2, 12, 0), 2);
    EXPECT_EQ(traitCoefficient(2, 12, 10000), 12);
    EXPECT_THROW(traitCoefficient(2, 1, 10), std::invalid_argument);
    EXPECT_THROW(traitCoefficient(0, 1, 10001), std::invalid_argument);
    EXPECT_THROW(traitCoefficient(0, std::numeric_limits<std::int64_t>::max(), 10000),
                 std::overflow_error);
}
