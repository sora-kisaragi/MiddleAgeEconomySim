#include <gtest/gtest.h>
#include "../../include/market/loan.h"

// Loan構造体の基本テスト
TEST(LoanTest, Creation) {
    Loan loan;
    
    // デフォルト値の確認
    EXPECT_EQ(loan.lender_id, 0);
    EXPECT_EQ(loan.borrower_id, 0);
    EXPECT_EQ(loan.amount, 0);
    EXPECT_FLOAT_EQ(loan.interest_rate, 0.0f);
    EXPECT_EQ(loan.days_remaining, 0);
    EXPECT_EQ(loan.payment_schedule, 0);
    EXPECT_FALSE(loan.defaulted);
}

// ローン情報設定のテスト
TEST(LoanTest, SetLoanInfo) {
    Loan loan;
    
    // ローン情報を設定
    loan.lender_id = 1;
    loan.borrower_id = 2;
    loan.amount = 1000;
    loan.interest_rate = 0.05f;
    loan.days_remaining = 30;
    loan.payment_schedule = 10;
    
    EXPECT_EQ(loan.lender_id, 1);
    EXPECT_EQ(loan.borrower_id, 2);
    EXPECT_EQ(loan.amount, 1000);
    EXPECT_FLOAT_EQ(loan.interest_rate, 0.05f);
    EXPECT_EQ(loan.days_remaining, 30);
    EXPECT_EQ(loan.payment_schedule, 10);
    EXPECT_FALSE(loan.defaulted);
}

// 利子計算のテスト
TEST(LoanTest, InterestCalculation) {
    Loan loan;
    loan.amount = 1000;
    loan.interest_rate = 0.05f; // 5%
    
    // 単純な利子計算（元本 * 利率）
    float interest = loan.amount * loan.interest_rate;
    EXPECT_FLOAT_EQ(interest, 50.0f);
}

// デフォルト状態の管理テスト
TEST(LoanTest, DefaultManagement) {
    Loan loan;
    
    // 初期状態では未デフォルト
    EXPECT_FALSE(loan.defaulted);
    
    // デフォルト状態に設定
    loan.defaulted = true;
    EXPECT_TRUE(loan.defaulted);
}

// 返済期間の管理テスト
TEST(LoanTest, PaymentPeriodManagement) {
    Loan loan;
    loan.days_remaining = 30;
    
    // 返済期間を減少
    loan.days_remaining -= 1;
    EXPECT_EQ(loan.days_remaining, 29);
    
    // 返済期限切れの確認
    loan.days_remaining = 0;
    EXPECT_EQ(loan.days_remaining, 0);
}