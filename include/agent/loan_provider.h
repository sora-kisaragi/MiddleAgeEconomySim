#pragma once
#include <vector>
#include "agent.h"
#include "../market/loan.h"

struct LoanProvider : public Agent {
    std::vector<Loan> active_loans;
    float base_interest_rate;
    
    // デフォルトコンストラクタ
    LoanProvider() : 
        base_interest_rate(0.05f) // 5%のデフォルト利子率
    {}
};