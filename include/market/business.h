#pragma once
#include <string>

struct Business {
    int id;
    std::string product;
    int stock;
    int price;
    int workers;
    int daily_production;
    
    // デフォルトコンストラクタ
    Business() : 
        id(0), 
        product(""), 
        stock(0), 
        price(0), 
        workers(0), 
        daily_production(0)
    {}
};
