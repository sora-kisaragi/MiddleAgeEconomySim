---
title: 🐛 Price volatility compounds indefinitely causing economic instability
labels: bug, critical, stability, math
assignees: none
---

## Description
The `price_volatility` variable in the Market class is multiplied by 1.1 without upper bounds, which can lead to extreme price fluctuations and numerical instability.

## Location
- **File**: `include/market/market.h`
- **Lines**: 47, 62 (in `addStock()` and `addDemand()` methods)

## Current Behavior
```cpp
price_volatility *= 1.1f;  // No upper bound!
```

## Expected Behavior
Price volatility should have reasonable bounds to maintain economic stability.

## Impact
- Extreme price fluctuations after prolonged simulation
- Numerical instability in price calculations  
- Unrealistic economic behavior
- Potential integer overflow in price values

## Reproduction Steps
1. Run simulation for many days (>20)
2. Create frequent supply/demand imbalances
3. Observe price volatility growing exponentially
4. Prices become unrealistically volatile

## Proposed Solution
```cpp
// Add upper bound and decay mechanism
if (quantity < latest_demand) {
    price_volatility = std::min(2.0f, price_volatility * 1.05f);
}
// Add decay over time
price_volatility = std::max(0.1f, price_volatility * 0.999f);
```

## Priority
🔴 **Critical** - Affects core simulation stability

