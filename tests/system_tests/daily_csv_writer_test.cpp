#include <gtest/gtest.h>
#include <sstream>
#include "system/daily_csv_writer.h"

TEST(DailyCsvWriterTest, CompletedDay_WritesHeaderAndExactColumns) {
    std::ostringstream output;
    DailyCsvWriter writer(output);
    writer.writeDay(1, {123.25, -0.125}, 7000);
    writer.writeDay(2, {100, 0}, std::numeric_limits<int64_t>::max());
    EXPECT_EQ(output.str(), "day,price_index,change_percent,total_money\n1,123.25,-0.125,7000\n2,100,0,9223372036854775807\n");
}

TEST(DailyCsvWriterTest, CustomLocale_StillWritesDecimalPoint) {
    class CommaDecimal : public std::numpunct<char> {
        char do_decimal_point() const override { return ','; }
        char do_thousands_sep() const override { return '.'; }
        std::string do_grouping() const override { return "\3"; }
    };
    std::ostringstream output;
    output.imbue(std::locale(std::locale::classic(), new CommaDecimal));
    DailyCsvWriter writer(output);
    writer.writeDay(1, {1000.5, 0.25}, 7000);
    EXPECT_EQ(output.str(), "day,price_index,change_percent,total_money\n1,1000.5,0.25,7000\n");
}

TEST(DailyCsvWriterTest, DoubleValues_RoundTripWithoutLosingPrecision) {
    std::ostringstream output;
    DailyCsvWriter writer(output);
    const PriceIndexObservation observation{100.0 / 3, -100.0 / 7};
    writer.writeDay(1, observation, 7000);
    const auto row = output.str().substr(output.str().find('\n') + 1);
    const auto first = row.find(',');
    const auto second = row.find(',', first + 1);
    const auto third = row.find(',', second + 1);
    EXPECT_EQ(std::stod(row.substr(first + 1, second - first - 1)), observation.value);
    EXPECT_EQ(std::stod(row.substr(second + 1, third - second - 1)), observation.change_percent);
}

TEST(DailyCsvWriterTest, StreamFailure_IsReportedAtHeaderAndDayWrite) {
    std::ostringstream failed;
    failed.setstate(std::ios::badbit);
    EXPECT_THROW(DailyCsvWriter writer(failed), std::runtime_error);
    std::ostringstream output;
    DailyCsvWriter writer(output);
    output.setstate(std::ios::badbit);
    EXPECT_THROW(writer.writeDay(1, {100, 0}, 7000), std::runtime_error);
}
