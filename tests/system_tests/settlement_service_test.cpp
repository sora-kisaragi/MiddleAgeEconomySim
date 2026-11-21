#include <gtest/gtest.h>
#include "core/settlement_service.h"

#include <limits>
#include <stdexcept>

using namespace economy;

namespace {
class SettlementServiceTest : public ::testing::Test {
protected:
    ActorRegistry registry = ActorRegistry::fromSpecs({
        {"person", "household", ActorKind::Person},
        {"shop", "shop", ActorKind::Business, "person", true},
        {"lender", "lender", ActorKind::Lender},
        {"government", "government", ActorKind::Government}
    });
    ActorId person = registry.actorByKey("person").id;
    ActorId shop = registry.actorByKey("shop").id;
    ActorId government = registry.actorByKey("government").id;
    AccountId household = registry.accountFor(person).id;
    AccountId business = registry.accountFor(shop).id;
    AccountId treasury = registry.accountFor(government).id;

    CashTransferCommand command(std::uint64_t id = 1,
                                std::uint64_t version = 0) const {
        return {CommandId(id), std::string(64, 'a'), version,
                GameTime(1, 2, id), person, person, household,
                {{business, Money(30)}, {treasury, Money(10)}}};
    }

    void expectInitial(const SettlementService& service) const {
        EXPECT_EQ(service.balance(household).value(), 100);
        EXPECT_EQ(service.balance(business).value(), 20);
        EXPECT_EQ(service.balance(treasury).value(), 0);
        EXPECT_EQ(service.stateVersion(), 0);
        EXPECT_EQ(service.totalCash().value(), 120);
    }
};
} // namespace

TEST_F(SettlementServiceTest, CommitsBatchAndKeepsHouseholdSeparateFromBusiness) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    const auto result = service.execute(command());
    EXPECT_EQ(result.status, SettlementStatus::Succeeded);
    EXPECT_EQ(result.reason, SettlementReason::None);
    EXPECT_EQ(result.amount.value(), 40);
    EXPECT_EQ(result.version_before, 0);
    EXPECT_EQ(result.version_after, 1);
    EXPECT_EQ(service.balance(household).value(), 60);
    EXPECT_EQ(service.balance(business).value(), 50);
    EXPECT_EQ(service.balance(treasury).value(), 10);
    EXPECT_EQ(service.totalCash().value(), 120);
    EXPECT_EQ(service.processedCount(), 1);
    EXPECT_EQ(registry.population(), 1);
}

TEST_F(SettlementServiceTest, SuccessReplayReturnsOriginalResultAfterLaterCommand) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    const auto original = service.execute(command());
    ASSERT_EQ(service.execute(command(2, 1)).status, SettlementStatus::Succeeded);
    const auto replay = service.execute(command());
    EXPECT_EQ(replay.command_id, original.command_id);
    EXPECT_EQ(replay.status, original.status);
    EXPECT_EQ(replay.version_before, original.version_before);
    EXPECT_EQ(replay.version_after, original.version_after);
    EXPECT_EQ(replay.amount, original.amount);
    EXPECT_EQ(service.balance(household).value(), 20);
    EXPECT_EQ(service.stateVersion(), 2);
    EXPECT_EQ(service.processedCount(), 2);
}

TEST_F(SettlementServiceTest, RejectionReplayDoesNotAddHistoryOrRetryChangedFunds) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    auto rejected = command();
    rejected.payments = {{business, Money(101)}};
    const auto first = service.execute(rejected);
    ASSERT_EQ(first.reason, SettlementReason::InsufficientFunds);
    auto funding = command(2);
    funding.actor_id = shop;
    funding.source_account = business;
    funding.payments = {{household, Money(20)}};
    ASSERT_EQ(service.execute(funding).status, SettlementStatus::Succeeded);
    EXPECT_EQ(service.execute(rejected).reason, SettlementReason::InsufficientFunds);
    EXPECT_EQ(service.balance(household).value(), 120);
    EXPECT_EQ(service.processedCount(), 2);
    auto retry = command(3, 1);
    retry.payments = rejected.payments;
    ASSERT_EQ(service.execute(retry).status, SettlementStatus::Succeeded);
    EXPECT_EQ(service.balance(household).value(), 19);
}

TEST_F(SettlementServiceTest, SameIdDifferentPayloadConflictsEvenWhenHashMatches) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    const auto original = command();
    ASSERT_EQ(service.execute(original).status, SettlementStatus::Succeeded);
    auto changed = original;
    changed.payments[0].amount = Money(1);
    EXPECT_EQ(service.execute(changed).reason, SettlementReason::IdConflict);
    auto changedHash = command();
    changedHash.request_hash[0] = 'b';
    EXPECT_EQ(service.execute(changedHash).reason, SettlementReason::IdConflict);
    EXPECT_EQ(service.processedCount(), 1);
    EXPECT_EQ(service.result(CommandId(1))->status, SettlementStatus::Succeeded);
    EXPECT_FALSE(service.result(CommandId(999)).has_value());
    EXPECT_EQ(service.balance(household).value(), 60);
}

TEST_F(SettlementServiceTest, LateInvalidReferenceDiscardsDebitAndEarlierCredit) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    auto input = command();
    input.payments[1].account_id = AccountId(999);
    EXPECT_EQ(service.execute(input).reason, SettlementReason::InvalidReference);
    expectInitial(service);
    EXPECT_EQ(service.processedCount(), 1);
    EXPECT_EQ(service.execute(input).reason, SettlementReason::InvalidReference);
    EXPECT_EQ(service.processedCount(), 1);
}

TEST_F(SettlementServiceTest, RejectsUnauthorizedSourceAndController) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    auto input = command();
    input.source_account = business;
    EXPECT_EQ(service.execute(input).reason, SettlementReason::Unauthorized);
    auto controller = command(2);
    controller.controller_person_id = government;
    EXPECT_EQ(service.execute(controller).reason, SettlementReason::Unauthorized);
    expectInitial(service);
}

TEST_F(SettlementServiceTest, ReadsLiveRegistryAndRejectsClosedBusiness) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    registry.closeBusiness(shop);
    EXPECT_EQ(service.execute(command()).reason, SettlementReason::InactiveActor);
    expectInitial(service);
}

TEST_F(SettlementServiceTest, RejectsInvalidRequestAndStaleVersion) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    auto empty = command();
    empty.payments.clear();
    EXPECT_EQ(service.execute(empty).reason, SettlementReason::InvalidRequest);
    auto hash = command(2);
    hash.request_hash = "bad";
    EXPECT_EQ(service.execute(hash).reason, SettlementReason::InvalidRequest);
    EXPECT_EQ(service.execute(command(3, 1)).reason, SettlementReason::StaleState);
    auto zero = command(4);
    zero.payments[1].amount = Money(0);
    EXPECT_EQ(service.execute(zero).reason, SettlementReason::InvalidRequest);
    auto self = command(5);
    self.payments[1].account_id = household;
    EXPECT_EQ(service.execute(self).reason, SettlementReason::InvalidRequest);
    expectInitial(service);
}

TEST_F(SettlementServiceTest, RejectsInvalidTimeAndBackwardTime) {
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}});
    auto zero = command();
    auto initial = CashTransferCommand{CommandId(1), zero.request_hash, 0,
        GameTime(0, 0, 0), person, person, household, zero.payments};
    EXPECT_EQ(service.execute(initial).reason, SettlementReason::InvalidTime);
    ASSERT_EQ(service.execute(command(2)).status, SettlementStatus::Succeeded);
    auto backwards = CashTransferCommand{CommandId(3), zero.request_hash, 1,
        GameTime(1, 2, 1), person, person, household, zero.payments};
    EXPECT_EQ(service.execute(backwards).reason, SettlementReason::InvalidTime);
    EXPECT_EQ(service.stateVersion(), 1);
}

TEST_F(SettlementServiceTest, RejectsAggregateOverflowAndAllowsExactMaximum) {
    const auto max = std::numeric_limits<std::int64_t>::max();
    SettlementService service(registry, {{household, Money(max)}});
    auto input = command();
    input.payments = {{business, Money(max)}, {treasury, Money(1)}};
    EXPECT_EQ(service.execute(input).reason, SettlementReason::AmountOverflow);
    EXPECT_EQ(service.balance(household).value(), max);
    input.command_id = CommandId(2);
    input.payments.pop_back();
    ASSERT_EQ(service.execute(input).status, SettlementStatus::Succeeded);
    EXPECT_EQ(service.balance(household).value(), 0);
    EXPECT_EQ(service.balance(business).value(), max);
    EXPECT_EQ(service.totalCash().value(), max);
}

TEST_F(SettlementServiceTest, ConstructionRejectsUnknownAccountsAndOverflow) {
    EXPECT_THROW((SettlementService(registry, {{AccountId(999), Money(1)}})), std::out_of_range);
    EXPECT_THROW((SettlementService(registry,
        {{household, Money(std::numeric_limits<std::int64_t>::max())}, {business, Money(1)}})),
        std::overflow_error);
}

TEST_F(SettlementServiceTest, InjectedExceptionsNeverPublishCandidateOrResult) {
    for (const auto failAt : {SettlementStage::AfterDebit,
                             SettlementStage::AfterCredit,
                             SettlementStage::BeforePublish}) {
        bool fail = true;
        SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}},
            [&](SettlementStage stage) {
                if (fail && stage == failAt) throw std::runtime_error("injected failure");
            });
        EXPECT_THROW(service.execute(command()), std::runtime_error);
        expectInitial(service);
        EXPECT_EQ(service.processedCount(), 0);
        fail = false;
        EXPECT_EQ(service.execute(command()).status, SettlementStatus::Succeeded);
        EXPECT_EQ(service.processedCount(), 1);
    }
}

TEST_F(SettlementServiceTest, FailureKeepsPreviouslyCommittedTransferAndAllowsRetry) {
    bool fail = false;
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}},
        [&](SettlementStage stage) {
            if (fail && stage == SettlementStage::BeforePublish) throw std::bad_alloc();
        });
    ASSERT_EQ(service.execute(command()).status, SettlementStatus::Succeeded);
    fail = true;
    EXPECT_THROW(service.execute(command(2, 1)), std::bad_alloc);
    EXPECT_EQ(service.balance(household).value(), 60);
    EXPECT_EQ(service.balance(business).value(), 50);
    EXPECT_EQ(service.processedCount(), 1);
    EXPECT_EQ(service.stateVersion(), 1);
    fail = false;
    EXPECT_EQ(service.execute(command(2, 1)).status, SettlementStatus::Succeeded);
}

TEST_F(SettlementServiceTest, RejectsReentrantExecutionFromFaultProbe) {
    SettlementService* active = nullptr;
    SettlementService service(registry, {{household, Money(100)}, {business, Money(20)}},
        [&](SettlementStage) { active->execute(command(2)); });
    active = &service;
    EXPECT_THROW(service.execute(command()), std::logic_error);
    expectInitial(service);
    EXPECT_EQ(service.processedCount(), 0);
}

TEST_F(SettlementServiceTest, RepeatedRecipientAndInstitutionTransfersPreserveCash) {
    SettlementService service(registry, {{treasury, Money(100)}});
    auto subsidy = command();
    subsidy.actor_id = government;
    subsidy.controller_person_id = std::nullopt;
    subsidy.source_account = treasury;
    subsidy.payments = {{household, Money(20)}, {household, Money(30)}, {business, Money(50)}};
    EXPECT_EQ(service.execute(subsidy).status, SettlementStatus::Succeeded);
    EXPECT_EQ(service.balance(household).value(), 50);
    EXPECT_EQ(service.balance(business).value(), 50);
    EXPECT_EQ(service.balance(treasury).value(), 0);
    EXPECT_EQ(service.totalCash().value(), 100);
}

TEST_F(SettlementServiceTest, RejectedResultPublicationFailureDoesNotConsumeCommandId) {
    bool fail = true;
    SettlementService service(registry, {{household, Money(100)}},
        [&](SettlementStage stage) {
            if (fail && stage == SettlementStage::BeforePublish) {
                throw std::out_of_range("injected result allocation failure");
            }
        });
    auto input = command();
    input.controller_person_id = std::nullopt;
    EXPECT_THROW(service.execute(input), std::out_of_range);
    EXPECT_EQ(service.processedCount(), 0);
    EXPECT_EQ(service.balance(household).value(), 100);
    fail = false;
    EXPECT_EQ(service.execute(input).reason, SettlementReason::Unauthorized);
    EXPECT_EQ(service.processedCount(), 1);
}

TEST_F(SettlementServiceTest, PhaseChangesCannotReuseOrReverseDailySequence) {
    SettlementService service(registry, {{household, Money(100)}});
    const auto transfer = [&](std::uint64_t id, std::uint64_t version,
                              std::uint64_t day, unsigned int phase,
                              std::uint64_t sequence) {
        return CashTransferCommand{CommandId(id), std::string(64, 'a'), version,
            GameTime(day, phase, sequence), person, person, household,
            {{business, Money(1)}}};
    };
    ASSERT_EQ(service.execute(transfer(1, 0, 1, 2, 5)).status, SettlementStatus::Succeeded);
    EXPECT_EQ(service.execute(transfer(2, 1, 1, 3, 5)).reason, SettlementReason::InvalidTime);
    EXPECT_EQ(service.execute(transfer(3, 1, 1, 3, 4)).reason, SettlementReason::InvalidTime);
    EXPECT_EQ(service.execute(transfer(4, 1, 1, 1, 6)).reason, SettlementReason::InvalidTime);
    EXPECT_EQ(service.stateVersion(), 1);
    EXPECT_EQ(service.balance(household).value(), 99);
    ASSERT_EQ(service.execute(transfer(5, 1, 1, 3, 6)).status, SettlementStatus::Succeeded);
    ASSERT_EQ(service.execute(transfer(6, 2, 1, 3, 7)).status, SettlementStatus::Succeeded);
    ASSERT_EQ(service.execute(transfer(7, 3, 2, 1, 1)).status, SettlementStatus::Succeeded);
    EXPECT_EQ(service.balance(household).value(), 96);
    EXPECT_EQ(service.totalCash().value(), 100);
}
