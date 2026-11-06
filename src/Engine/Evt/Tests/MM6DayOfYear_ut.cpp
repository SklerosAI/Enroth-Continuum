#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6DayOfYear.h"

UNIT_TEST(MM6DayOfYear, ComparesOneBasedDaysAcrossYearBoundary) {
    EXPECT_TRUE(mm6::compareDayOfYear(Time::fromDays(0), 19, 1));
    EXPECT_TRUE(mm6::compareDayOfYear(Time::fromDays(83), 19, 84));
    EXPECT_FALSE(mm6::compareDayOfYear(Time::fromDays(83), 19, 83));
    EXPECT_TRUE(mm6::compareDayOfYear(Time::fromDays(335), 19, 336));
    EXPECT_TRUE(mm6::compareDayOfYear(Time::fromDays(336), 19, 1));
    EXPECT_TRUE(mm6::compareDayOfYear(Time::fromDays(671), 19, 336));
}

UNIT_TEST(MM6DayOfYear, InvalidInputRejects) {
    EXPECT_THROW(mm6::compareDayOfYear(Time::fromDays(0), 18, 1), std::runtime_error);
    EXPECT_THROW(mm6::compareDayOfYear(Time::fromDays(0), 19, 0), std::runtime_error);
    EXPECT_THROW(mm6::compareDayOfYear(Time::fromDays(0), 19, 337), std::runtime_error);
    EXPECT_THROW(mm6::compareDayOfYear(Time::fromTicks(-1), 19, 1), std::runtime_error);
}
