#include <gtest/gtest.h>
#include "agent/government.h"
#include "agent/person.h"
#include "market/business.h"

class GovernmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        government = new Government();
        citizen = new Person();
        business = new Business();
        
        // 基本設定
        government->tax_rate = 10; // 10%
        government->money = 1000;  // treasuryの代わりにmoneyを使用
        government->sector_subsidies["agriculture"] = 100.0f; // 補助金額を設定
        
        citizen->money = 1000;
        
        business->money = 2000;
        business->profit_margin = 0.2f; // 20%
        business->sector = "agriculture"; // セクターを設定
    }

    void TearDown() override {
        delete government;
        delete citizen;
        delete business;
    }

    Government* government;
    Person* citizen;
    Business* business;
};

TEST_F(GovernmentTest, CollectTax_FromPerson) {
    bool result = government->collectTax(citizen);
    
    EXPECT_TRUE(result);
    // 1000の10% = 100が徴収される
    EXPECT_EQ(citizen->money, 900);
    EXPECT_EQ(government->money, 1100);
}

TEST_F(GovernmentTest, CollectTax_FromBusiness) {
    bool result = government->collectTax(business);
    
    EXPECT_TRUE(result);
    // 2000の10% = 200が徴収される
    EXPECT_EQ(business->money, 1800);
    EXPECT_EQ(government->money, 1200);
}

TEST_F(GovernmentTest, CollectTax_InsufficientFunds) {
    citizen->money = 50; // 極めて少額に設定
    bool result = government->collectTax(citizen);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(citizen->money, 50); // 変化なし
    EXPECT_EQ(government->money, 1000); // 変化なし
}

TEST_F(GovernmentTest, ImplementPolicy_Subsidy) {
    std::string policy = "subsidy";
    bool result = government->implementPolicy(policy, business);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(business->money, 2100); // 補助金100が追加
    EXPECT_EQ(government->money, 900); // treasuryの代わりにmoneyを使用
}

TEST_F(GovernmentTest, ImplementPolicy_PriceControl) {
    std::string policy = "price_control";
    business->product = "grain";
    business->price = 100;
    
    bool result = government->implementPolicy(policy, business);
    
    EXPECT_TRUE(result);
    EXPECT_LE(business->price, 100); // 価格が制御されている
}

TEST_F(GovernmentTest, UpdateApprovalRating) {
    // 初期承認率を設定
    government->approval_rating = 50.0f;
    
    // 政策実施による承認率の変化をテスト
    EXPECT_TRUE(government->implementPolicy("subsidy", business));
    float after_subsidy = government->approval_rating;
    EXPECT_GT(after_subsidy, 50.0f); // 補助金支給で承認率上昇
    
    // 増税による承認率の変化をテスト
    government->tax_rate = 20; // 税率を上げる
    government->updateApprovalRating();
    EXPECT_LT(government->approval_rating, after_subsidy); // 増税で承認率低下
}