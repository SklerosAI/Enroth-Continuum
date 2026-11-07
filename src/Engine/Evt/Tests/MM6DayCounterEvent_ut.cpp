#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6DayCounterEvent.h"

UNIT_TEST(MM6DayCounterEvent, CountsCalendarDaysSinceRestart) {
    // MM6 variables 216-221 count calendar days, not 24-hour periods.
    EXPECT_EQ(mm6::dayCounterIndex(216), 0);
    EXPECT_EQ(mm6::dayCounterIndex(221), 5);
    EXPECT_THROW(mm6::dayCounterIndex(215), std::runtime_error);
    EXPECT_THROW(mm6::dayCounterIndex(222), std::runtime_error);

    Time started = Time::fromDays(10) + Duration::fromHours(23);
    EXPECT_TRUE(mm6::compareDayCounter(started, started, 0));
    EXPECT_FALSE(mm6::compareDayCounter(started, started, 1));
    EXPECT_TRUE(mm6::compareDayCounter(started, Time::fromDays(11), 1));
    EXPECT_FALSE(mm6::compareDayCounter(started, Time::fromDays(40) + Duration::fromHours(23), 31));
    EXPECT_TRUE(mm6::compareDayCounter(started, Time::fromDays(41), 31));
}

UNIT_TEST(MM6DayCounterEvent, ClearedCounterCountsFromTheStartOfTime) {
    // A cleared MM6 day counter compares against day zero.
    EXPECT_TRUE(mm6::compareDayCounter(Time(), Time::fromDays(31), 31));
    EXPECT_FALSE(mm6::compareDayCounter(Time(), Time::fromDays(30), 31));
    EXPECT_THROW(mm6::compareDayCounter(Time::fromTicks(-1), Time::fromDays(1), 0), std::runtime_error);
}
