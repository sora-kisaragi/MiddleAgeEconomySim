#ifndef AGENT_H
#define AGENT_H

#include <limits>
#include <stdexcept>
#include <cstdint>

class Agent {
public:
    int64_t id;
    int64_t money;

    Agent() : id(0), money(0) {}

    void addMoney(int64_t amount) {
        if (amount > 0 && money > std::numeric_limits<int64_t>::max() - amount) {
            throw std::overflow_error("Money addition would cause overflow");
        }
        if (amount < 0 && money < std::numeric_limits<int64_t>::min() - amount) {
            throw std::underflow_error("Money subtraction would cause underflow");
        }
        money += amount;
    }
};

#endif // AGENT_H