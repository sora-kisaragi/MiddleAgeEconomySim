#include <gtest/gtest.h>
#include "system/trade_route.h"

TEST(TradeRouteTest, DefaultConstructor) {
    TradeRoute tr;
    EXPECT_EQ(0, tr.from_location_id);
    EXPECT_EQ(0, tr.to_location_id);
    EXPECT_TRUE(tr.goods.empty());
    EXPECT_EQ(0, tr.travel_time);
}

TEST(TradeRouteTest, CustomTradeRoute) {
    TradeRoute tr;
    tr.from_location_id = 1;
    tr.to_location_id = 2;
    tr.goods["Wheat"] = 100;
    tr.travel_time = 3;
    
    EXPECT_EQ(1, tr.from_location_id);
    EXPECT_EQ(2, tr.to_location_id);
    EXPECT_EQ(1, tr.goods.size());
    EXPECT_EQ(100, tr.goods["Wheat"]);
    EXPECT_EQ(3, tr.travel_time);
}

TEST(TradeRouteTest, MultipleGoods) {
    TradeRoute tr;
    tr.from_location_id = 1;
    tr.to_location_id = 2;
    tr.goods["Wheat"] = 100;
    tr.goods["Bread"] = 50;
    tr.travel_time = 3;
    
    EXPECT_EQ(1, tr.from_location_id);
    EXPECT_EQ(2, tr.to_location_id);
    EXPECT_EQ(2, tr.goods.size());
    EXPECT_EQ(100, tr.goods["Wheat"]);
    EXPECT_EQ(50, tr.goods["Bread"]);
    EXPECT_EQ(3, tr.travel_time);
}
