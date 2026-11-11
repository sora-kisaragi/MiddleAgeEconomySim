#ifndef GOVERNMENT_H
#define GOVERNMENT_H

#include <algorithm>
#include <cmath>
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
        return collectTaxFrom(citizen, 100);
    }

    bool collectTax(Business* business) {
        return collectTaxFrom(business, 1000);
    }

    bool implementPolicy(const std::string& policy, Business* target) {
        if (!target) return false;

        if (policy == "subsidy") {
            auto subsidy = sector_subsidies.find(target->sector);
            if (subsidy != sector_subsidies.end()) {
                const float value = subsidy->second;
                // 2^63 is the first unrepresentable positive int64_t value.
                if (!std::isfinite(value) || value < 1.0f ||
                    static_cast<double>(value) >= 9223372036854775808.0) return false;
                const int64_t amount = static_cast<int64_t>(value);
                if (money < amount || target->money > std::numeric_limits<int64_t>::max() - amount) {
                    return false;
                }
                // Allocate history before transferring; allocation failure leaves cash unchanged.
                policies.push_back(policy);
                if (!transferMoneyTo(target, amount)) {
                    policies.pop_back();
                    return false;
                }
                approval_rating = std::clamp(approval_rating + 5.0f, 0.0f, 100.0f);
                return true;
            }
        }
        else if (policy == "price_control") {
            if (target->price > 0) {
                policies.push_back(policy);
                target->price = (target->price / 10) * 9 + (target->price % 10) * 9 / 10;
                approval_rating = std::clamp(approval_rating + 3.0f, 0.0f, 100.0f); // 価格統制による即時の承認率上昇
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
private:
    bool collectTaxFrom(Agent* payer, int64_t minimum_balance) {
        if (!payer || payer->money <= minimum_balance || tax_rate < 0 || tax_rate > 100) {
            return false;
        }
        const int64_t tax = (payer->money / 100) * tax_rate +
                            (payer->money % 100) * tax_rate / 100;
        if (!payer->transferMoneyTo(this, tax)) return false;
        approval_rating = std::clamp(approval_rating - 1.0f, 0.0f, 100.0f);
        return true;
    }

};

#endif // GOVERNMENT_H