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