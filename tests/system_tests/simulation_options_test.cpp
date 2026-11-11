#include <gtest/gtest.h>
#include "system/simulation_options.h"

TEST(SimulationOptionsTest, NoArguments_SelectsFiveDayDemo) {
    const auto options = parseSimulationOptions({});
    EXPECT_EQ(options.scenario, "demo");
    EXPECT_EQ(options.days, 5);
    EXPECT_FALSE(options.help);
}

TEST(SimulationOptionsTest, ScenarioAndDays_AreParsed) {
    for (const char* scenario : {"demo", "balanced", "shortage", "oversupply"}) {
        const auto options = parseSimulationOptions({"--scenario", scenario, "--days", "3"});
        EXPECT_EQ(options.scenario, scenario);
        EXPECT_EQ(options.days, 3);
    }
    EXPECT_EQ(parseSimulationOptions({"--days", "10000"}).days, 10000);
    EXPECT_TRUE(parseSimulationOptions({"--help"}).help);
}

TEST(SimulationOptionsTest, InvalidArguments_AreRejected) {
    const std::vector<std::vector<std::string>> invalid = {
        {"--days"}, {"--scenario"}, {"--unknown"}, {"--scenario", "typo"},
        {"--days", "0"}, {"--days", "-1"}, {"--days", "10001"},
        {"--days", "3days"}, {"--days", "99999999999999999999999"}, {"--days", ""}
    };
    for (const auto& arguments : invalid) {
        EXPECT_THROW(parseSimulationOptions(arguments), std::invalid_argument);
    }
}
