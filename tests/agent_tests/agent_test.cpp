#include <gtest/gtest.h>
#include "../../include/agent/agent.h"
#include "../../include/market/transaction.h"

// Agent構造体の基本テスト
TEST(AgentTest, Creation) {
    Agent agent;
    EXPECT_EQ(agent.id, 0);
    EXPECT_EQ(agent.money, 0);
    EXPECT_TRUE(agent.history.empty());
}

// メモリ管理のテスト
TEST(AgentTest, CopyConstruction) {
    Agent original;
    original.id = 1;
    original.money = 100;
    
    Transaction* trans = new Transaction(1, 2, 3, "Test", 1, 10);
    original.addTransaction(trans);
    
    // コピー構築
    Agent copy(original);
    
    EXPECT_EQ(copy.id, original.id);
    EXPECT_EQ(copy.money, original.money);
    EXPECT_EQ(copy.history.size(), original.history.size());
    
    // 深いコピーの確認
    EXPECT_NE(copy.history[0], original.history[0]);
    EXPECT_EQ(copy.history[0]->transaction_id, original.history[0]->transaction_id);
}

// ムーブのテスト
TEST(AgentTest, MoveConstruction) {
    Agent original;
    original.id = 1;
    original.money = 100;
    
    Transaction* trans = new Transaction(1, 2, 3, "Test", 1, 10);
    original.addTransaction(trans);
    
    // ムーブ構築
    Agent moved(std::move(original));
    
    EXPECT_EQ(moved.id, 1);
    EXPECT_EQ(moved.money, 100);
    EXPECT_EQ(moved.history.size(), 1);
    
    // 元のオブジェクトが空になっていることを確認
    EXPECT_EQ(original.id, 0);
    EXPECT_EQ(original.money, 0);
    EXPECT_TRUE(original.history.empty());
}

// お金の操作テスト
TEST(AgentTest, MoneyOperations) {
    Agent agent;
    
    // 正常な追加
    EXPECT_NO_THROW(agent.addMoney(100));
    EXPECT_EQ(agent.money, 100);
    
    // 負の値の追加
    EXPECT_NO_THROW(agent.addMoney(-50));
    EXPECT_EQ(agent.money, 50);
    
    // オーバーフローのテスト
    EXPECT_THROW(agent.addMoney(std::numeric_limits<int64_t>::max()), std::overflow_error);
    
    // アンダーフローのテスト
    EXPECT_THROW(agent.addMoney(std::numeric_limits<int64_t>::min()), std::underflow_error);
}

// 取引履歴管理のテスト
TEST(AgentTest, TransactionManagement) {
    Agent agent;
    
    // nullptrの追加を試みる
    EXPECT_THROW(agent.addTransaction(nullptr), std::invalid_argument);
    
    // 正常な取引の追加
    Transaction* trans = new Transaction(1, 2, 3, "Test", 1, 10);
    EXPECT_NO_THROW(agent.addTransaction(trans));
    EXPECT_EQ(agent.history.size(), 1);
    
    // 履歴のクリア
    agent.clearHistory();
    EXPECT_TRUE(agent.history.empty());
}

// デストラクタと記憶リークのテスト
TEST(AgentTest, Destruction) {
    Agent* agent = new Agent();
    Transaction* trans = new Transaction(1, 2, 3, "Test", 1, 10);
    agent->addTransaction(trans);
    
    // デストラクタが呼ばれ、メモリリークが起きないことを確認
    delete agent;  // CIでメモリリークチェッカーを使用して確認
}