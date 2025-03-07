#ifndef GOVERNMENT_H
#define GOVERNMENT_H

#include <string>
#include <vector>
#include <map>
#include "agent.h"
#include "person.h"
#include "../market/business.h"

class Government : public Agent {
public:
    int tax_rate;
    float approval_rating;
    std::vector<std::string> policies;
    std::map<std::string, float> sector_subsidies;

    Government() : tax_rate(10), approval_rating(50.0f) {}

    bool collectTax(Person* citizen) {
        if (!citizen) return false;
        
        // 税額を計算（切り捨て）
        int64_t tax_amount = (citizen->money * tax_rate) / 100;
        
        // 最低生存費用（100）未満の場合は徴収不可
        if (citizen->money <= 100) {
            return false;
        }
        
        citizen->addMoney(-tax_amount);
        addMoney(tax_amount);
        approval_rating -= 1.0f; // 徴税による承認率低下
        return true;
    }

    bool collectTax(Business* business) {
        if (!business) return false;
        
        // 税額を計算（切り捨て）
        int64_t tax_amount = (business->money * tax_rate) / 100;
        
        // 最低運営資金（1000）未満の場合は徴収不可
        if (business->money <= 1000) {
            return false;
        }
        
        business->addMoney(-tax_amount);
        addMoney(tax_amount);
        approval_rating -= 1.0f; // 徴税による承認率低下
        return true;
    }

    bool implementPolicy(const std::string& policy, Business* target) {
        if (!target) return false;

        if (policy == "subsidy") {
            auto subsidy = sector_subsidies.find(target->sector);
            if (subsidy != sector_subsidies.end()) {
                int amount = static_cast<int>(subsidy->second);
                if (money >= amount) {
                    target->addMoney(amount);
                    addMoney(-amount);
                    policies.push_back(policy);
                    approval_rating += 5.0f; // 補助金政策による即時の承認率上昇
                    return true;
                }
            }
        }
        else if (policy == "price_control") {
            if (target->price > 0) {
                target->price = static_cast<int>(target->price * 0.9);
                policies.push_back(policy);
                approval_rating += 3.0f; // 価格統制による即時の承認率上昇
                return true;
            }
        }
        return false;
    }

    void updateApprovalRating() {
        // 税率の影響を評価（15%を超える分について）
        if (tax_rate > 15) {
            approval_rating -= (tax_rate - 15) * 2.0f;
        }
        
        // 承認率の範囲制限
        if (approval_rating > 100.0f) approval_rating = 100.0f;
        if (approval_rating < 0.0f) approval_rating = 0.0f;
    }
};

#endif // GOVERNMENT_H