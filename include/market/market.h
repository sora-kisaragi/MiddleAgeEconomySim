#pragma once
#include <map>
#include <string>

struct Market {
    std::map<std::string, int> stock;
    std::map<std::string, int> price;
    
    // デフォルトコンストラクタ
    Market() = default;
};
