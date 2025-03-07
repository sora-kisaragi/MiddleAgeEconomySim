#ifndef LOAN_H
#define LOAN_H

#include <cstdint>

struct Loan {
    int64_t lender_id;       // 貸し手のID
    int64_t borrower_id;     // 借り手のID
    int64_t amount;          // 融資額
    float interest_rate;     // 利子率
    int32_t days_remaining;  // 残り日数
    int32_t payment_schedule; // 返済スケジュール（日数）
    bool defaulted;          // デフォルト状態

    Loan() :
        lender_id(0),
        borrower_id(0),
        amount(0),
        interest_rate(0.0f),
        days_remaining(0),
        payment_schedule(0),  // 初期値を0に変更
        defaulted(false)
    {}
};

#endif // LOAN_H