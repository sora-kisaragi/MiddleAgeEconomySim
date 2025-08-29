#pragma once
#ifndef LOAN_PROVIDER_H
#define LOAN_PROVIDER_H

#include <vector>
#include "agent.h"
#include "../market/loan.h"

class LoanProvider : public Agent {
public:
    float base_interest_rate;
    std::vector<Loan> active_loans;

    LoanProvider() : base_interest_rate(0.05f) {}

    bool provideLoan(Agent* borrower, int64_t amount) {
        if (!borrower || amount <= 0) return false;
        if (money < amount) return false;

        Loan loan;
        loan.lender_id = id;
        loan.borrower_id = borrower->id;
        loan.amount = amount;
        loan.interest_rate = base_interest_rate;
        loan.days_remaining = 30;  // 30日の融資期間
        loan.defaulted = false;

        money -= amount;
        borrower->addMoney(amount);
        active_loans.push_back(loan);

        return true;
    }

    bool collectInterest() {
        bool all_collected = true;
        for (auto& loan : active_loans) {
            if (loan.defaulted) continue;

            Agent* borrower = findBorrower(loan.borrower_id);
            if (!borrower) {
                loan.defaulted = true;
                all_collected = false;
                continue;
            }

            int64_t interest = static_cast<int64_t>(loan.amount * loan.interest_rate);
            if (borrower->money < interest) {
                loan.defaulted = true;
                all_collected = false;
                continue;
            }

            borrower->addMoney(-interest);
            addMoney(interest);
            loan.days_remaining--;
        }
        return all_collected;
    }

private:
    Agent* findBorrower(int64_t /*borrower_id*/) {
        // この実装は後で適切なものに置き換える必要がある
        return nullptr;
    }
};

#endif // LOAN_PROVIDER_H