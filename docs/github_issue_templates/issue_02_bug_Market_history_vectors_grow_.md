---
title: 🐛 Market history vectors grow indefinitely causing memory leak
labels: bug, performance, memory
assignees: none
---

## Description
The `demand_history` and `supply_history` vectors in the Market class grow indefinitely without size limits, which could lead to memory exhaustion in long-running simulations.

## Location
- **File**: `include/market/market.h`
- **Lines**: 14-15 (declarations), 41, 56 (usage)

## Current Behavior
```cpp
demand_history[product].push_back(quantity);  // Grows forever
supply_history[product].push_back(quantity);  // Grows forever
```

## Expected Behavior
History should have reasonable size limits or use circular buffers.

## Impact
- Memory usage grows linearly with simulation duration
- Potential out-of-memory crashes in long simulations
- Degraded performance due to large vector operations

## Proposed Solution
1. **Option 1: Circular Buffer**
```cpp
const size_t MAX_HISTORY = 100;
if (demand_history[product].size() >= MAX_HISTORY) {
    demand_history[product].erase(demand_history[product].begin());
}
demand_history[product].push_back(quantity);
```

2. **Option 2: Periodic Cleanup**
```cpp
// In clearDaily() or periodic maintenance
if (demand_history[product].size() > MAX_HISTORY) {
    demand_history[product].resize(MAX_HISTORY/2);
}
```

## Priority
🟡 **High** - Affects long-running simulations

