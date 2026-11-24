#include "system/runtime_timeline.h"
#include "system/simulation_options.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(RuntimeTimelineTest, StableActorOrderAndDayBoundarySequences) {
    std::ostringstream first, second;
    RuntimeTimeline a(first, {economy::ActorId(20), economy::ActorId(10)});
    RuntimeTimeline b(second, {economy::ActorId(10), economy::ActorId(20)});
    for (auto* trace : {&a, &b}) {
        EXPECT_EQ(trace->beginDay(1), economy::GameTime(1, 1, 1));
        trace->completeDay(1);
        EXPECT_EQ(trace->beginDay(2), economy::GameTime(2, 1, 1));
        trace->completeDay(2);
    }
    EXPECT_EQ(first.str(), second.str());
    EXPECT_EQ(first.str(), "event_id,event_type,actor_id,day,phase,sequence_in_day\n"
        "1,ACTOR_REGISTERED,10,0,0,1\n2,ACTOR_REGISTERED,20,0,0,2\n"
        "3,DAY_STARTED,,1,1,1\n4,DAY_COMPLETED,,1,12,2\n"
        "5,DAY_STARTED,,2,1,1\n6,DAY_COMPLETED,,2,12,2\n");
}

TEST(RuntimeTimelineTest, InvalidLifecycleDoesNotAppendRows) {
    std::ostringstream output;
    RuntimeTimeline trace(output, {});
    const auto header = output.str();
    EXPECT_THROW(trace.beginDay(0), std::invalid_argument);
    EXPECT_THROW(trace.beginDay(2), std::invalid_argument);
    EXPECT_THROW(trace.completeDay(1), std::invalid_argument);
    EXPECT_EQ(output.str(), header);
    trace.beginDay(1);
    const auto opened = output.str();
    EXPECT_THROW(trace.beginDay(2), std::invalid_argument);
    EXPECT_THROW(trace.completeDay(2), std::invalid_argument);
    EXPECT_EQ(output.str(), opened);
    trace.completeDay(1);
    const auto completed = output.str();
    EXPECT_THROW(trace.completeDay(1), std::invalid_argument);
    EXPECT_THROW(trace.beginDay(1), std::invalid_argument);
    EXPECT_EQ(output.str(), completed);
}

TEST(RuntimeTimelineTest, DuplicateActorsAndWriteFailuresAreRejected) {
    std::ostringstream output;
    EXPECT_THROW((RuntimeTimeline(output, {economy::ActorId(1), economy::ActorId(1)})),
                 std::invalid_argument);
    EXPECT_TRUE(output.str().empty());
    RuntimeTimeline trace(output, {});
    trace.beginDay(1);
    const auto before = output.str();
    output.setstate(std::ios::badbit);
    EXPECT_THROW(trace.completeDay(1), std::runtime_error);
    EXPECT_EQ(output.str(), before);
    EXPECT_EQ(before.find("DAY_COMPLETED"), std::string::npos);
}

TEST(RuntimeTimelineTest, DecimalIdsAndTimelineOptionAreExplicit) {
    std::ostringstream output;
    output << std::hex;
    RuntimeTimeline trace(output, {economy::ActorId(20)});
    EXPECT_NE(output.str().find("ACTOR_REGISTERED,20,"), std::string::npos);
    EXPECT_TRUE(parseSimulationOptions({}).timeline_path.empty());
    EXPECT_EQ(parseSimulationOptions({"--timeline", "trace.csv"}).timeline_path, "trace.csv");
    EXPECT_THROW(parseSimulationOptions({"--timeline"}), std::invalid_argument);
    EXPECT_THROW(parseSimulationOptions({"--timeline", ""}), std::invalid_argument);
}
