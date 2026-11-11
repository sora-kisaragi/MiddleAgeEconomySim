#pragma once
#include <string>
#include <vector>

struct SimulationOptions {
    std::string scenario = "demo";
    std::string csv_path;
    int days = 5;
    bool help = false;
};

SimulationOptions parseSimulationOptions(const std::vector<std::string>& arguments);
