#pragma once
#include <map>
#include <string>

struct Market {
    std::map<std::string, int> stock;
    std::map<std::string, int> price;
    std::map<std::string, int> demand_history;
    std::map<std::string, int> supply_history;
    float price_volatility;

    // デフォルトコンストラクタ
    Market() : price_volatility(0.0) {}

  };
