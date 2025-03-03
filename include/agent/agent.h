#pragma once
#include <vector>

// 前方宣言
struct Transaction;

struct Agent {
    int id;
    int money;
    std::vector<Transaction*> history;
    
    // デフォルトコンストラクタ
    Agent() : 
        id(0), 
        money(0)
    {}
    
    // 仮想デストラクタ（将来的なクラス化に備えて）
    virtual ~Agent() {}
};