#include "system/simulation_options.h"
#include <charconv>
#include <stdexcept>

SimulationOptions parseSimulationOptions(const std::vector<std::string>& arguments) {
    SimulationOptions options;
    for (size_t index = 0; index < arguments.size(); ++index) {
        const auto& argument = arguments[index];
        if (argument == "--help") {
            options.help = true;
        } else if (argument == "--days" || argument == "--scenario") {
            if (++index == arguments.size()) throw std::invalid_argument("Option requires a value");
            const auto& value = arguments[index];
            if (argument == "--days") {
                const auto result = std::from_chars(value.data(), value.data() + value.size(), options.days);
                if (result.ec != std::errc() || result.ptr != value.data() + value.size() ||
                    options.days < 1 || options.days > 10000) {
                    throw std::invalid_argument("Days must be an integer from 1 to 10000");
                }
            } else {
                if (value != "demo" && value != "balanced" && value != "shortage" && value != "oversupply") {
                    throw std::invalid_argument("Unknown economic scenario");
                }
                options.scenario = value;
            }
        } else {
            throw std::invalid_argument("Unknown option: " + argument);
        }
    }
    return options;
}
