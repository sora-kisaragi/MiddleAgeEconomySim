#include "agent/agent.h"

#include <gtest/gtest.h>
#include <limits>

TEST(TransferBalanceValidationTest, NegativeRecipientRejectsPaymentWithoutChangingEitherBalance) {
    for (int64_t balance : {int64_t{-1}, std::numeric_limits<int64_t>::min()}) {
        for (int64_t amount : {int64_t{0}, int64_t{1}, std::numeric_limits<int64_t>::max()}) {
            SCOPED_TRACE(::testing::Message() << "balance=" << balance << " amount=" << amount);
            Agent payer;
            Agent recipient;
            payer.money = std::numeric_limits<int64_t>::max();
            recipient.money = balance;
            EXPECT_FALSE(payer.transferMoneyTo(&recipient, amount));
            EXPECT_EQ(payer.money, std::numeric_limits<int64_t>::max());
            EXPECT_EQ(recipient.money, balance);
        }
    }
}

TEST(TransferBalanceValidationTest, EconomicEntryPointsRejectNegativeRecipientBalance) {
    Agent payer;
    Agent recipient;
    payer.money = 10;
    recipient.money = -1;
    EXPECT_FALSE(payer.directTrade(&recipient, "grain", 1, 1));
    EXPECT_EQ(payer.money, 10);
    EXPECT_EQ(recipient.money, -1);
    EXPECT_FALSE(recipient.provideService(&payer, "work", 1));
    EXPECT_EQ(payer.money, 10);
    EXPECT_EQ(recipient.money, -1);
    EXPECT_FALSE(recipient.requestLoan(&payer, 1, 0.0f));
    EXPECT_EQ(payer.money, 10);
    EXPECT_EQ(recipient.money, -1);
}
