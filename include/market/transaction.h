#pragma once
#include <string>
#include <ctime>

struct Transaction {
    int transaction_id;
    int seller_id;
    int buyer_id;
    std::string item;
    int quantity;
    int price;
    time_t timestamp;
    
    // デフォルトコンストラクタ
    Transaction() : 
        transaction_id(0), 
        seller_id(0), 
        buyer_id(0), 
        item(""), 
        quantity(0), 
        price(0), 
        timestamp(0)
    {}
};