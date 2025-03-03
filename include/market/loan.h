#pragma once

struct Loan {
    int lender_id;
    int borrower_id;
    int amount;
    float interest_rate;
    int days_remaining;
    int payment_schedule;
    bool defaulted;
    
    // デフォルトコンストラクタ
    Loan() : 
        lender_id(0), 
        borrower_id(0), 
        amount(0), 
        interest_rate(0.0f), 
        days_remaining(0), 
        payment_schedule(0), 
        defaulted(false)
    {}
};