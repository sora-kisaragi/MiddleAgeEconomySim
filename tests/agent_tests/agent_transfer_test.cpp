#include <gtest/gtest.h>
#include <limits>
#include "agent/agent.h"

TEST(AgentTransferTest, DirectTrade_RecipientOverflow_LeavesBothBalancesUnchanged) {
    Agent buyer;
    Agent seller;
    buyer.money = 100;
    seller.money = std::numeric_limits<int64_t>::max();
    EXPECT_FALSE(buyer.directTrade(&seller, "grain", 1, 10));
    EXPECT_EQ(buyer.money, 100);
    EXPECT_EQ(seller.money, std::numeric_limits<int64_t>::max());
}

TEST(AgentTransferTest, RequestLoan_BorrowerOverflow_LeavesBothBalancesUnchanged) {
    Agent borrower;
    Agent lender;
    borrower.money = std::numeric_limits<int64_t>::max();
    lender.money = 100;
    EXPECT_FALSE(borrower.requestLoan(&lender, 10, 0.0f));
    EXPECT_EQ(borrower.money, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(lender.money, 100);
}

TEST(AgentTransferTest, Service_ProviderOverflow_LeavesBothBalancesUnchanged) {
    Agent provider;
    Agent client;
    provider.money = std::numeric_limits<int64_t>::max();
    client.money = 100;
    EXPECT_FALSE(provider.provideService(&client, "consultation", 10));
    EXPECT_EQ(provider.money, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(client.money, 100);
}

TEST(AgentTransferTest, DirectTrade_InvalidQuantityOrPrice_IsRejected) {
    Agent buyer;
    Agent seller;
    buyer.money = 100;
    for (int quantity : {0, -1}) {
        EXPECT_FALSE(buyer.directTrade(&seller, "grain", quantity, 10));
        EXPECT_EQ(buyer.money, 100);
        EXPECT_EQ(seller.money, 0);
    }
    EXPECT_FALSE(buyer.directTrade(&seller, "grain", 1, -10));
    EXPECT_EQ(buyer.money, 100);
    EXPECT_EQ(seller.money, 0);
}

TEST(AgentTransferTest, DirectTrade_OverflowingTotal_IsRejected) {
    Agent buyer;
    Agent seller;
    buyer.money = std::numeric_limits<int64_t>::max();
    EXPECT_FALSE(buyer.directTrade(&seller, "grain", 2, std::numeric_limits<int64_t>::max()));
    EXPECT_EQ(buyer.money, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(seller.money, 0);
}

TEST(AgentTransferTest, DirectTrade_MaximumRepresentableTotal_Succeeds) {
    Agent buyer;
    Agent seller;
    buyer.money = std::numeric_limits<int64_t>::max();
    EXPECT_TRUE(buyer.directTrade(&seller, "grain", 1, buyer.money));
    EXPECT_EQ(buyer.money, 0);
    EXPECT_EQ(seller.money, std::numeric_limits<int64_t>::max());
}

TEST(AgentTransferTest, ZeroCostTradeAndService_PreserveBalances) {
    Agent buyer;
    Agent seller;
    EXPECT_TRUE(buyer.directTrade(&seller, "grain", 1, 0));
    EXPECT_TRUE(seller.provideService(&buyer, "advice", 0));
    EXPECT_EQ(buyer.money, 0);
    EXPECT_EQ(seller.money, 0);
}

TEST(AgentTransferTest, Loan_NonFiniteRate_IsRejected) {
    Agent borrower;
    Agent lender;
    lender.money = 100;
    for (float rate : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        EXPECT_FALSE(borrower.requestLoan(&lender, 10, rate));
        EXPECT_EQ(borrower.money, 0);
        EXPECT_EQ(lender.money, 100);
    }
}
