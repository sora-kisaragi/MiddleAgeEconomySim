#include "agent/agent.h"
#include <iostream>

bool Agent::directTrade(Agent* other, const std::string& item, int quantity, int64_t price) {
    if (!canInteractWith(other)) {
        return false;
    }
    
    int64_t total_cost = price * quantity;
    
    // Check if this agent has enough money to buy
    if (money < total_cost) {
        return false;
    }
    
    // Simple trade implementation - could be enhanced based on agent types
    try {
        // Execute the trade
        addMoney(-total_cost);
        other->addMoney(total_cost);
        
        std::cout << "Direct trade completed: " << item << " (" << quantity 
                  << " units) for " << total_cost << " coins" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Trade failed: " << e.what() << std::endl;
        return false;
    }
}

bool Agent::requestLoan(Agent* lender, int64_t amount, float interest_rate) {
    if (!canInteractWith(lender) || amount <= 0 || interest_rate < 0) {
        return false;
    }
    
    // Check if lender has enough money
    if (lender->money < amount) {
        return false;
    }
    
    try {
        // Simple loan implementation
        lender->addMoney(-amount);
        addMoney(amount);
        
        std::cout << "Loan granted: " << amount << " coins at " 
                  << interest_rate << "% interest" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Loan request failed: " << e.what() << std::endl;
        return false;
    }
}

bool Agent::provideService(Agent* client, const std::string& service, int64_t cost) {
    if (!canInteractWith(client) || cost < 0) {
        return false;
    }
    
    // Check if client has enough money
    if (client->money < cost) {
        return false;
    }
    
    try {
        // Execute service transaction
        client->addMoney(-cost);
        addMoney(cost);
        
        std::cout << "Service provided: " << service << " for " << cost 
                  << " coins" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Service provision failed: " << e.what() << std::endl;
        return false;
    }
}

size_t Agent::getTransactionCount() const {
    return 0; // Simplified for now
}

void Agent::clearOldTransactions(size_t max_history) {
    // Simplified for now
}