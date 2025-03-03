#pragma once
#include <string>
#include "../agent/agent.h"

struct Business : public Agent {
    std::string product;
    int stock;
    int price;
    int workers;
    int daily_production;
    float profit_margin;
    int market_share;
    
    // デフォルトコンストラクタ
    Business() :
        product(""),
        stock(0),
        price(0),
        workers(0),
        daily_production(0),
        profit_margin(0.0),
        market_share(0)
    {}
};
