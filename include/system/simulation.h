#pragma once
#include <vector>
#include "agent/person.h"
#include "agent/government.h"
#include "agent/loan_provider.h"
#include "market/business.h"
#include "market/market.h"
#include "system/trade_route.h"

void simulateDay(std::vector<Person>& people, std::vector<Business>& businesses, Market& market,
                 Government& government, LoanProvider& loan_provider, std::vector<TradeRoute>& trade_routes);
