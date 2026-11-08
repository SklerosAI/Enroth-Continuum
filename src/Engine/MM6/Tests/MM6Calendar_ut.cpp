#include "Testing/Unit/UnitTest.h"

#include "Core/Time/Time.h"

#include "Engine/MM6/MM6Calendar.h"

UNIT_TEST(MM6Calendar, CivilYearCountsFromTheTitleStartingYear) {
    // An MM6 calendar starts three years before MM7's, everything below the year is the same.
    const Time time = Time(0, 30, 9, 3, 1, 4, 2);

    const CivilTime mm7 = time.toCivilTime();
    EXPECT_EQ(gameStartingYear, 1168);
    EXPECT_EQ(mm7.year, 1170);
    EXPECT_EQ(Time().toCivilTime().year, 1168);

    const CivilTime mm6 = time.toCivilTime(mm6::calendarStartingYear);
    EXPECT_EQ(mm6.year, 1167);
    EXPECT_EQ(Time().toCivilTime(mm6::calendarStartingYear).year, 1165);

    EXPECT_EQ(mm6.month, mm7.month);
    EXPECT_EQ(mm6.month, 5);
    EXPECT_EQ(mm6.day, mm7.day);
    EXPECT_EQ(mm6.day, 11);
    EXPECT_EQ(mm6.dayOfWeek, mm7.dayOfWeek);
    EXPECT_EQ(mm6.hour, mm7.hour);
    EXPECT_EQ(mm6.minute, mm7.minute);
}
