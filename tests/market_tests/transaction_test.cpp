#include <gtest/gtest.h>
#include "../../include/market/transaction.h"
#include <thread>
#include <chrono>

// Transaction構造体の基本テスト
TEST(TransactionTest, Creation) {
    Transaction transaction;
    
    // デフォルト値の確認
    EXPECT_EQ(transaction.transaction_id, 0);
    EXPECT_EQ(transaction.seller_id, 0);
    EXPECT_EQ(transaction.buyer_id, 0);
    EXPECT_EQ(transaction.item, "");
    EXPECT_EQ(transaction.quantity, 0);
    EXPECT_EQ(transaction.price, 0);
    EXPECT_GT(transaction.timestamp, 0);  // 現在時刻が設定されていることを確認
}

// 完全コンストラクタのテスト
TEST(TransactionTest, CompleteConstruction) {
    EXPECT_NO_THROW({
        Transaction trans(1, 2, 3, "Test Item", 5, 100);
        EXPECT_EQ(trans.transaction_id, 1);
        EXPECT_EQ(trans.seller_id, 2);
        EXPECT_EQ(trans.buyer_id, 3);
        EXPECT_EQ(trans.item, "Test Item");
        EXPECT_EQ(trans.quantity, 5);
        EXPECT_EQ(trans.price, 100);
        EXPECT_GT(trans.timestamp, 0);
    });
}

// バリデーションのテスト
TEST(TransactionTest, Validation) {
    // 不正なID
    EXPECT_THROW(Transaction(-1, 2, 3, "Test", 5, 100), std::invalid_argument);
    EXPECT_THROW(Transaction(1, -2, 3, "Test", 5, 100), std::invalid_argument);
    EXPECT_THROW(Transaction(1, 2, -3, "Test", 5, 100), std::invalid_argument);
    
    // 空のアイテム名
    EXPECT_THROW(Transaction(1, 2, 3, "", 5, 100), std::invalid_argument);
    
    // 不正な数量
    EXPECT_THROW(Transaction(1, 2, 3, "Test", 0, 100), std::invalid_argument);
    EXPECT_THROW(Transaction(1, 2, 3, "Test", -1, 100), std::invalid_argument);
    
    // 不正な価格
    EXPECT_THROW(Transaction(1, 2, 3, "Test", 5, -1), std::invalid_argument);
}

// 取引情報設定のテスト
TEST(TransactionTest, SetTransactionInfo) {
    Transaction transaction;
    
    // 取引情報を設定
    transaction.transaction_id = 1001;
    transaction.seller_id = 5;
    transaction.buyer_id = 10;
    transaction.item = "Wheat";
    transaction.quantity = 50;
    transaction.price = 100;
    transaction.timestamp = 1234567890;
    
    EXPECT_EQ(transaction.transaction_id, 1001);
    EXPECT_EQ(transaction.seller_id, 5);
    EXPECT_EQ(transaction.buyer_id, 10);
    EXPECT_EQ(transaction.item, "Wheat");
    EXPECT_EQ(transaction.quantity, 50);
    EXPECT_EQ(transaction.price, 100);
    EXPECT_EQ(transaction.timestamp, 1234567890);
}

// タイムスタンプ設定のテスト
TEST(TransactionTest, TimestampManagement) {
    Transaction transaction;
    
    // 現在時刻を設定
    time_t now = time(nullptr);
    transaction.timestamp = now;
    
    EXPECT_EQ(transaction.timestamp, now);
}

// 取引総額計算のテスト
TEST(TransactionTest, TotalValueCalculation) {
    Transaction transaction;
    
    transaction.quantity = 5;
    transaction.price = 20;
    
    // 総取引額の計算（quantity * price）
    int total_value = transaction.quantity * transaction.price;
    
    EXPECT_EQ(total_value, 100);
}

// 取引額計算のテスト
TEST(TransactionTest, ValueCalculation) {
    Transaction trans(1, 2, 3, "Test", 5, 100);
    EXPECT_EQ(trans.getTotalValue(), 500);  // 5 * 100
    
    // オーバーフローのテスト
    Transaction large_trans(1, 2, 3, "Test", std::numeric_limits<int32_t>::max(), std::numeric_limits<int64_t>::max() / 1000);
    EXPECT_THROW(large_trans.getTotalValue(), std::overflow_error);
}

// タイムスタンプ機能のテスト
TEST(TransactionTest, TimestampFunctionality) {
    Transaction trans;
    time_t start = std::time(nullptr);
    
    // 1秒待機
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    time_t elapsed = trans.getElapsedTime();
    EXPECT_GE(elapsed, 1);
    EXPECT_TRUE(trans.isOlderThan(1));
    EXPECT_FALSE(trans.isOlderThan(100));
}

// セッターメソッドのテスト
TEST(TransactionTest, Setters) {
    Transaction trans;
    
    // 正常なケース
    EXPECT_NO_THROW(trans.setQuantity(10));
    EXPECT_EQ(trans.quantity, 10);
    
    EXPECT_NO_THROW(trans.setPrice(200));
    EXPECT_EQ(trans.price, 200);
    
    EXPECT_NO_THROW(trans.setItem("New Item"));
    EXPECT_EQ(trans.item, "New Item");
    
    // エラーケース
    EXPECT_THROW(trans.setQuantity(0), std::invalid_argument);
    EXPECT_THROW(trans.setQuantity(-1), std::invalid_argument);
    EXPECT_THROW(trans.setPrice(-1), std::invalid_argument);
    EXPECT_THROW(trans.setItem(""), std::invalid_argument);
}