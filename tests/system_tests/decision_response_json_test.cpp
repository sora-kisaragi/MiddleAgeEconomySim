#include "decision/decision_response_json.h"

#include <gtest/gtest.h>

using namespace economy;

namespace {
const std::string hash(64, 'a');

std::string response(const std::string& candidate = "buy") {
    return "{\"schema_version\":\"decision.v1\",\"decision_id\":\"day8-phase08-actor17-food-attempt1\","
           "\"input_hash\":\"" + hash + "\",\"candidate_id\":\"" + candidate +
           "\",\"reason_code\":\"FOOD_PRIORITY\"}";
}
} // namespace

TEST(DecisionResponseJsonTest, ParsesExactShapeAndSelectsStoredCandidate) {
    const DecisionRequest request("run-42", "day8-phase08-actor17-food-attempt1", hash, ActorId(17), ActorId(17),
        GameTime(8, 8, 1), 4, {{"pass", PassAction{"NO_ACTION"}},
        {"buy", PlaceOrderAction{IntentId(91), QuoteId(40), Quantity(3), Money(30), true}}});
    const auto parsed = parseDecisionResponseJson(response());
    const auto selected = selectCandidate(request, parsed, 4);
    EXPECT_EQ(std::get<PlaceOrderAction>(selected.action).quote_id.value(), 40U);
}

TEST(DecisionResponseJsonTest, RejectsUnknownDuplicateMissingAndExtraFields) {
    EXPECT_THROW(parseDecisionResponseJson(response().substr(0, response().size() - 1) + ",\"extra\":\"x\"}"),
                 std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response().substr(0, response().size() - 1) +
                 ",\"candidate_id\":\"pass\"}"), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson("{\"schema_version\":\"decision.v1\"}"), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response().substr(0, response().size() - 1) +
                 ",\"candid\\u0061te_id\":\"pass\"}"), std::invalid_argument);
}

TEST(DecisionResponseJsonTest, RejectsWrongTypesAndMultipleDocuments) {
    auto with_number = response();
    const auto pos = with_number.find("\"candidate_id\":\"buy\"");
    with_number.replace(pos, std::string("\"candidate_id\":\"buy\"").size(), "\"candidate_id\":123");
    EXPECT_THROW(parseDecisionResponseJson(with_number), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response() + response()), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response() + " []"), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response().substr(0, response().size() - 1) + ",}"),
                 std::invalid_argument);
}

TEST(DecisionResponseJsonTest, RejectsInvalidVersionHashReasonAndCandidate) {
    auto bad = response();
    bad.replace(bad.find("decision.v1"), 11, "decision.v2");
    EXPECT_THROW(parseDecisionResponseJson(bad), std::invalid_argument);
    bad = response();
    bad.replace(bad.find(hash), 1, "A");
    EXPECT_THROW(parseDecisionResponseJson(bad), std::invalid_argument);
    bad = response();
    bad.replace(bad.find("FOOD_PRIORITY"), 13, "NEW_REASON");
    EXPECT_THROW(parseDecisionResponseJson(bad), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response("")), std::invalid_argument);
}

TEST(DecisionResponseJsonTest, HandlesEscapesAndRejectsMalformedUnicodeAndUtf8) {
    auto escaped = response("b\\u0075y");
    EXPECT_EQ(parseDecisionResponseJson(escaped).candidate_id, "buy");
    EXPECT_THROW(parseDecisionResponseJson(response("b\\uD800y")), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response("b\\uDC00y")), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response("b\\uD800\\u0061y")), std::invalid_argument);
    EXPECT_THROW(parseDecisionResponseJson(response("b\\x79")), std::invalid_argument);
    const std::string invalid_utf8(1, static_cast<char>(0xc0));
    EXPECT_THROW(parseDecisionResponseJson(response("b" + invalid_utf8)), std::invalid_argument);
    const std::string face = "\xf0\x9f\x98\x80";
    EXPECT_EQ(parseDecisionResponseJson(response("b\\uD83D\\uDE00")).candidate_id, "b" + face);
    EXPECT_TRUE(validDecisionId(std::string(128, 'a')));
    EXPECT_FALSE(validDecisionId(std::string(128, 'a') + face));
    std::string unicode_id;
    for (int i = 0; i < 128; ++i) unicode_id += face;
    EXPECT_TRUE(validDecisionId(unicode_id));
    EXPECT_FALSE(validDecisionId(unicode_id + face));
    EXPECT_FALSE(validDecisionId("b\n"));
    EXPECT_FALSE(validDecisionId(std::string("b\0", 2)));
    EXPECT_THROW(parseDecisionResponseJson(response("b\\u0000")), std::invalid_argument);
}
