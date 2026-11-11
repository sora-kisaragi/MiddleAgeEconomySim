#include <gtest/gtest.h>
#include <limits>
#include "agent/government.h"
#include "agent/loan_provider.h"

TEST(PublicFinanceTest, Tax_RecipientOverflow_PreservesBothBalances) {
    Government government;
    government.money = std::numeric_limits<int64_t>::max();
    Person citizen;
    citizen.money = 1000;
    EXPECT_FALSE(government.collectTax(&citizen));
    EXPECT_EQ(citizen.money, 1000);
    EXPECT_EQ(government.money, std::numeric_limits<int64_t>::max());
    EXPECT_FLOAT_EQ(government.approval_rating, 50.0f);
}

TEST(PublicFinanceTest, Tax_LargeBalance_ComputesExactAmountWithoutOverflow) {
    Government government;
    Person citizen;
    citizen.money = std::numeric_limits<int64_t>::max();
    EXPECT_TRUE(government.collectTax(&citizen));
    EXPECT_EQ(government.money, 922337203685477580LL);
    EXPECT_EQ(citizen.money, 8301034833169298227LL);
}

TEST(PublicFinanceTest, Tax_InvalidRates_AreRejectedForPeopleAndBusinesses) {
    Government government;
    Person citizen;
    Business business;
    citizen.money = business.money = 2000;
    for (int rate : {-1, 101, std::numeric_limits<int>::max()}) {
        government.tax_rate = rate;
        EXPECT_FALSE(government.collectTax(&citizen));
        EXPECT_FALSE(government.collectTax(&business));
        EXPECT_EQ(citizen.money, 2000);
        EXPECT_EQ(business.money, 2000);
        EXPECT_EQ(government.money, 0);
    }
}

TEST(PublicFinanceTest, Subsidy_InvalidAmounts_AreRejectedWithoutRecordingPolicy) {
    Government government;
    government.money = 1000;
    Business business;
    business.sector = "farm";
    for (float amount : {-1.0f, 0.0f, 0.5f, std::numeric_limits<float>::infinity(),
                         std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        government.sector_subsidies["farm"] = amount;
        EXPECT_FALSE(government.implementPolicy("subsidy", &business));
        EXPECT_EQ(government.money, 1000);
        EXPECT_EQ(business.money, 0);
        EXPECT_TRUE(government.policies.empty());
    }
}

TEST(PublicFinanceTest, Subsidy_RecipientOverflow_DoesNotRecordOrTransfer) {
    Government government;
    government.money = 1000;
    government.sector_subsidies["farm"] = 50;
    Business business;
    business.sector = "farm";
    business.money = std::numeric_limits<int64_t>::max();
    EXPECT_FALSE(government.implementPolicy("subsidy", &business));
    EXPECT_EQ(government.money, 1000);
    EXPECT_EQ(business.money, std::numeric_limits<int64_t>::max());
    EXPECT_TRUE(government.policies.empty());
}

TEST(PublicFinanceTest, PriceControl_LargePrice_StaysIn64BitRange) {
    Government government;
    Business business;
    business.price = std::numeric_limits<int64_t>::max();
    EXPECT_TRUE(government.implementPolicy("price_control", &business));
    EXPECT_EQ(business.price, 8301034833169298226LL);
}

TEST(PublicFinanceTest, ApprovalRating_RemainsWithinBounds) {
    Government government;
    government.money = 1000;
    government.approval_rating = 99;
    government.sector_subsidies["farm"] = 50;
    Business business;
    business.sector = "farm";
    EXPECT_TRUE(government.implementPolicy("subsidy", &business));
    EXPECT_FLOAT_EQ(government.approval_rating, 100.0f);
    government.approval_rating = 0;
    Person citizen;
    citizen.money = 1000;
    EXPECT_TRUE(government.collectTax(&citizen));
    EXPECT_FLOAT_EQ(government.approval_rating, 0.0f);
}

TEST(PublicFinanceTest, Loan_BorrowerOverflow_PreservesBalancesAndLoanList) {
    LoanProvider lender;
    lender.money = 100;
    Agent borrower;
    borrower.money = std::numeric_limits<int64_t>::max();
    EXPECT_FALSE(lender.provideLoan(&borrower, 10));
    EXPECT_EQ(lender.money, 100);
    EXPECT_EQ(borrower.money, std::numeric_limits<int64_t>::max());
    EXPECT_TRUE(lender.active_loans.empty());
}

TEST(PublicFinanceTest, Loan_InvalidRateOrSelf_IsRejected) {
    LoanProvider lender;
    lender.money = 100;
    EXPECT_FALSE(lender.provideLoan(&lender, 10));
    Agent borrower;
    for (float rate : {-1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        lender.base_interest_rate = rate;
        EXPECT_FALSE(lender.provideLoan(&borrower, 10));
        EXPECT_EQ(lender.money, 100);
        EXPECT_EQ(borrower.money, 0);
        EXPECT_TRUE(lender.active_loans.empty());
    }
}

TEST(PublicFinanceTest, Loan_MaximumPrincipal_IsRecordedAndTransferred) {
    LoanProvider lender;
    lender.money = std::numeric_limits<int64_t>::max();
    Agent borrower;
    EXPECT_TRUE(lender.provideLoan(&borrower, std::numeric_limits<int64_t>::max()));
    EXPECT_EQ(lender.money, 0);
    EXPECT_EQ(borrower.money, std::numeric_limits<int64_t>::max());
    ASSERT_EQ(lender.active_loans.size(), 1u);
    EXPECT_EQ(lender.active_loans[0].amount, std::numeric_limits<int64_t>::max());
}

TEST(PublicFinanceTest, Tax_RateEndpointsAndFractionalRounding) {
    Government government;
    Person citizen;
    citizen.money = 199;
    government.tax_rate = 0;
    EXPECT_TRUE(government.collectTax(&citizen));
    EXPECT_EQ(citizen.money, 199);
    government.tax_rate = 33;
    EXPECT_TRUE(government.collectTax(&citizen));
    EXPECT_EQ(government.money, 65);
    EXPECT_EQ(citizen.money, 134);
    government.tax_rate = 100;
    EXPECT_TRUE(government.collectTax(&citizen));
    EXPECT_EQ(government.money, 199);
    EXPECT_EQ(citizen.money, 0);
}

TEST(PublicFinanceTest, Subsidy_FractionAndLargeValidAmount) {
    Government government;
    Business business;
    business.sector = "farm";
    government.money = std::numeric_limits<int64_t>::max();
    government.sector_subsidies["farm"] = 50.75f;
    EXPECT_TRUE(government.implementPolicy("subsidy", &business));
    EXPECT_EQ(business.money, 50);
    const float large = std::nextafter(9223372036854775808.0f, 0.0f);
    government.sector_subsidies["farm"] = large;
    EXPECT_TRUE(government.implementPolicy("subsidy", &business));
    EXPECT_EQ(business.money, 50 + static_cast<int64_t>(large));
    EXPECT_EQ(government.money, std::numeric_limits<int64_t>::max() - business.money);
    government.sector_subsidies["farm"] = 9223372036854775808.0f;
    EXPECT_FALSE(government.implementPolicy("subsidy", &business));
    EXPECT_EQ(government.policies.size(), 2u);
}

TEST(PublicFinanceTest, Subsidy_InsufficientTreasury_PreservesPolicyAndApproval) {
    Government government;
    government.money = 49;
    government.sector_subsidies["farm"] = 50;
    Business business;
    business.sector = "farm";
    EXPECT_FALSE(government.implementPolicy("subsidy", &business));
    EXPECT_EQ(government.money, 49);
    EXPECT_EQ(business.money, 0);
    EXPECT_TRUE(government.policies.empty());
    EXPECT_FLOAT_EQ(government.approval_rating, 50.0f);
}
