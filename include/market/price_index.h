#pragma once
#include <map>
#include <string>
#include <stdexcept>
#include <utility>
#include "market.h"

struct PriceIndexObservation {
    double value;
    double change_percent;
};

class PriceIndex {
public:
    PriceIndex(const Market& market, std::map<std::string, int> quantities)
        : quantities(std::move(quantities)), previous_index(100.0) {
        if (this->quantities.empty()) throw std::invalid_argument("Price basket must not be empty");
        for (const auto& item : this->quantities) {
            if (item.second <= 0) throw std::invalid_argument("Basket quantities must be positive");
        }
        base_value = basketValue(market);
    }

    // Call once after each simulated day has closed.
    PriceIndexObservation observe(const Market& market) {
        const double current_index = basketValue(market) / base_value * 100.0;
        const double change = (current_index / previous_index - 1.0) * 100.0;
        previous_index = current_index;
        return {current_index, change};
    }

private:
    std::map<std::string, int> quantities;
    double base_value;
    double previous_index;

    double basketValue(const Market& market) const {
        double total = 0.0;
        for (const auto& [product, quantity] : quantities) {
            const int price = market.getPrice(product);
            if (price <= 0) throw std::invalid_argument("Basket product must have a positive price");
            total += static_cast<double>(price) * quantity;
        }
        return total;
    }
};
