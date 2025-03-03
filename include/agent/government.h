#pragma once
#include <vector>
#include <map>
#include <string>
#include "agent.h"

struct Government : public Agent {
    int tax_rate;
    std::vector<std::string> policies;
    int treasury;
    float approval_rating;
    std::map<std::string, float> sector_subsidies;
    
    // デフォルトコンストラクタ
    Government() : 
        tax_rate(10), // 10%のデフォルト税率
        treasury(0),
        approval_rating(50.0f) // 50%の初期支持率
    {}
};