#include <gtest/gtest.h>
#include "agent/agent.h"

// Agent構造体の基本テスト
TEST(AgentTest, Creation) {
    Agent agent;
    EXPECT_EQ(agent.id, 0);
    EXPECT_EQ(agent.money, 0);
}

// お金の操作テスト
TEST(AgentTest, MoneyManagement) {
    Agent agent;
    
    // 正常な金銭の追加
    agent.addMoney(1000);
    EXPECT_EQ(agent.money, 1000);
    
    // 負の金額の処理
    agent.addMoney(-500);
    EXPECT_EQ(agent.money, 500);
}

// オーバーフローのテスト
TEST(AgentTest, MoneyOverflow) {
    Agent agent;
    agent.money = std::numeric_limits<int64_t>::max() - 10;
    
    // オーバーフローのテスト
    EXPECT_THROW(agent.addMoney(100), std::overflow_error);
}

// アンダーフローのテスト
TEST(AgentTest, MoneyUnderflow) {
    Agent agent;
    agent.money = std::numeric_limits<int64_t>::min() + 10;
    
    // アンダーフローのテスト
    EXPECT_THROW(agent.addMoney(-100), std::underflow_error);
}

// エージェント相互作用のテスト
TEST(AgentTest, CanInteractWith) {
    Agent agent1, agent2;
    
    EXPECT_TRUE(agent1.canInteractWith(&agent2));
    EXPECT_TRUE(agent2.canInteractWith(&agent1));
    EXPECT_FALSE(agent1.canInteractWith(&agent1)); // Can't interact with self
    EXPECT_FALSE(agent1.canInteractWith(nullptr)); // Can't interact with null
}

TEST(AgentTest, DirectTrade_Success) {
    Agent agent1, agent2;
    agent1.money = 1000;
    agent2.money = 500;
    
    bool result = agent1.directTrade(&agent2, "grain", 10, 50);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(agent1.money, 500); // 1000 - (10 * 50)
    EXPECT_EQ(agent2.money, 1000); // 500 + (10 * 50)
}

TEST(AgentTest, DirectTrade_InsufficientFunds) {
    Agent agent1, agent2;
    agent1.money = 100;
    agent2.money = 500;
    
    bool result = agent1.directTrade(&agent2, "grain", 10, 50);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(agent1.money, 100); // No change
    EXPECT_EQ(agent2.money, 500); // No change
}

TEST(AgentTest, RequestLoan_Success) {
    Agent agent1, agent2;
    agent1.money = 100; // borrower
    agent2.money = 2000; // lender
    
    bool result = agent1.requestLoan(&agent2, 500, 5.0f);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(agent1.money, 600); // 100 + 500
    EXPECT_EQ(agent2.money, 1500); // 2000 - 500
}

TEST(AgentTest, RequestLoan_InsufficientFunds) {
    Agent agent1, agent2;
    agent1.money = 100; // borrower
    agent2.money = 200; // lender with insufficient funds
    
    bool result = agent1.requestLoan(&agent2, 500, 5.0f);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(agent1.money, 100); // No change
    EXPECT_EQ(agent2.money, 200); // No change
}

TEST(AgentTest, ProvideService_Success) {
    Agent agent1, agent2;
    agent1.money = 1000; // service provider
    agent2.money = 500; // client
    
    bool result = agent1.provideService(&agent2, "consultation", 200);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(agent1.money, 1200); // 1000 + 200
    EXPECT_EQ(agent2.money, 300); // 500 - 200
}

TEST(AgentTest, ProvideService_InsufficientFunds) {
    Agent agent1, agent2;
    agent1.money = 1000; // service provider
    agent2.money = 100; // client with insufficient funds
    
    bool result = agent1.provideService(&agent2, "consultation", 200);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(agent1.money, 1000); // No change
    EXPECT_EQ(agent2.money, 100); // No change
}

TEST(AgentTest, GetAgentType) {
    Agent agent;
    EXPECT_EQ(agent.getAgentType(), "Agent");
}

TEST(AgentTest, TransactionCount) {
    Agent agent;
    EXPECT_EQ(agent.getTransactionCount(), 0);
}