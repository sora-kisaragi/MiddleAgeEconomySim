#include <gtest/gtest.h>
#include "agent/loan_provider.h"
#include "agent/person.h"

class LoanProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        lender = new LoanProvider();
        borrower = new Person();
        
        // 基本設定
        lender->id = 1;
        lender->money = 1000;
        lender->base_interest_rate = 0.05f;
        
        borrower->id = 2;
        borrower->money = 100;
    }

    void TearDown() override {
        delete lender;
        delete borrower;
    }

    LoanProvider* lender;
    Person* borrower;
};

TEST_F(LoanProviderTest, Creation) {
    LoanProvider provider;
    EXPECT_EQ(provider.id, 0);
    EXPECT_EQ(provider.money, 0);
    EXPECT_TRUE(provider.active_loans.empty());
    EXPECT_FLOAT_EQ(provider.base_interest_rate, 0.05f);
}

TEST_F(LoanProviderTest, ProvideLoan_Success) {
    int loan_amount = 500;
    bool result = lender->provideLoan(borrower, loan_amount);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(lender->money, 500); // 1000 - 500
    EXPECT_EQ(borrower->money, 600); // 100 + 500
    EXPECT_EQ(lender->active_loans.size(), 1);
    
    const Loan& loan = lender->active_loans[0];
    EXPECT_EQ(loan.lender_id, lender->id);
    EXPECT_EQ(loan.borrower_id, borrower->id);
    EXPECT_EQ(loan.amount, loan_amount);
    EXPECT_FLOAT_EQ(loan.interest_rate, lender->base_interest_rate);
    EXPECT_FALSE(loan.defaulted);
}

TEST_F(LoanProviderTest, ProvideLoan_InsufficientFunds) {
    int loan_amount = 2000; // lenderの所持金以上
    bool result = lender->provideLoan(borrower, loan_amount);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(lender->money, 1000); // 変化なし
    EXPECT_EQ(borrower->money, 100); // 変化なし
    EXPECT_TRUE(lender->active_loans.empty());
}

TEST_F(LoanProviderTest, ProvideLoan_InvalidAmount) {
    bool result = lender->provideLoan(borrower, 0);
    EXPECT_FALSE(result);
    
    result = lender->provideLoan(borrower, -100);
    EXPECT_FALSE(result);
    
    EXPECT_EQ(lender->money, 1000); // 変化なし
    EXPECT_EQ(borrower->money, 100); // 変化なし
    EXPECT_TRUE(lender->active_loans.empty());
}

TEST_F(LoanProviderTest, ProvideLoan_NullBorrower) {
    bool result = lender->provideLoan(nullptr, 500);
    EXPECT_FALSE(result);
    EXPECT_EQ(lender->money, 1000); // 変化なし
    EXPECT_TRUE(lender->active_loans.empty());
}