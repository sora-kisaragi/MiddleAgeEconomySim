#ifndef AGENT_H
#define AGENT_H

#include <limits>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <string>

class Agent {
public:
    int64_t id;
    int64_t money;

    Agent() : id(0), money(0) {}
    virtual ~Agent() = default; // Add virtual destructor for polymorphic classes

    void addMoney(int64_t amount) {
        if (amount > 0 && money > std::numeric_limits<int64_t>::max() - amount) {
            throw std::overflow_error("Money addition would cause overflow");
        }
        if (amount < 0 && money < std::numeric_limits<int64_t>::min() - amount) {
            throw std::underflow_error("Money subtraction would cause underflow");
        }
        money += amount;
    }

    // Agent interaction methods
    virtual bool canInteractWith(const Agent* other) const {
        return other != nullptr && other != this;
    }

    virtual bool directTrade(Agent* other, const std::string& item, int quantity, int64_t price);
    virtual bool requestLoan(Agent* lender, int64_t amount, float interest_rate);
    virtual bool provideService(Agent* client, const std::string& service, int64_t cost);
    
    // Virtual methods for different agent types to override
    virtual std::string getAgentType() const { return "Agent"; }
    virtual void updateState() {}
    virtual void dailyUpdate() {}

    // Transaction history management
    size_t getTransactionCount() const;
    void clearOldTransactions(size_t max_history = 1000);
};

#endif // AGENT_H