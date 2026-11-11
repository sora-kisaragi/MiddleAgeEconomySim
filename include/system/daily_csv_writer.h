#pragma once
#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>
#include <stdexcept>
#include "market/price_index.h"

// Owns no stream. The caller keeps the destination alive for all completed days.
class DailyCsvWriter {
public:
    explicit DailyCsvWriter(std::ostream& destination) : destination(destination) {
        destination.imbue(std::locale::classic());
        destination << "day,price_index,change_percent,total_money\n";
        checkOutput();
    }

    void writeDay(int day, const PriceIndexObservation& observation, int64_t total_money) {
        destination << day << ',' << std::setprecision(std::numeric_limits<double>::max_digits10)
                    << observation.value << ',' << observation.change_percent << ',' << total_money << '\n';
        checkOutput();
    }

private:
    std::ostream& destination;
    void checkOutput() {
        destination.flush();
        if (!destination) throw std::runtime_error("Cannot write daily observation CSV");
    }
};
