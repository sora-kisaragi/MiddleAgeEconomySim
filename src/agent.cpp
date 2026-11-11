#include "agent/agent.h"
#include <cmath>
#include <iostream>

bool Agent::transferMoneyTo(Agent* recipient, int64_t amount) {
    if (!recipient || recipient == this || amount < 0 || money < amount ||
        recipient->money > std::numeric_limits<int64_t>::max() - amount) {
        return false;
    }
    // Both arithmetic operations are valid before either balance is changed.
    addMoney(-amount);
    recipient->addMoney(amount);
    return true;
}

bool Agent::directTrade(Agent* other, const std::string& item, int quantity, int64_t price) {
    if (!canInteractWith(other) || quantity <= 0 || price < 0 ||
        price > std::numeric_limits<int64_t>::max() / quantity) {
        return false;
    }
    const int64_t total_cost = price * quantity;
    if (!transferMoneyTo(other, total_cost)) return false;
    std::cout << "Direct trade completed: " << item << " (" << quantity
              << " units) for " << total_cost << " coins" << std::endl;
    return true;
}

bool Agent::requestLoan(Agent* lender, int64_t amount, float interest_rate) {
    if (!canInteractWith(lender) || amount <= 0 || !std::isfinite(interest_rate) || interest_rate < 0) {
        return false;
    }
    if (!lender->transferMoneyTo(this, amount)) return false;
    std::cout << "Loan granted: " << amount << " coins at "
              << interest_rate << "% interest" << std::endl;
    return true;
}

bool Agent::provideService(Agent* client, const std::string& service, int64_t cost) {
    if (!canInteractWith(client) || cost < 0) return false;
    if (!client->transferMoneyTo(this, cost)) return false;
    std::cout << "Service provided: " << service << " for " << cost << " coins" << std::endl;
    return true;
}

size_t Agent::getTransactionCount() const {
    return 0; // Simplified for now
}

void Agent::clearOldTransactions(size_t max_history) {
    // Simplified for now
}