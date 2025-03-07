#pragma once
#include <map>
#include <string>

struct TradeRoute {
    int from_location_id;
    int to_location_id;
    std::map<std::string, int> goods;
    int travel_time;
    
    // デフォルトコンストラクタ
    TradeRoute() : 
        from_location_id(0), 
        to_location_id(0), 
        travel_time(0)
    {}
};
