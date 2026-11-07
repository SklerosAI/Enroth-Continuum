#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6VisitedReturnPreEventProbeForTest.h"

UNIT_TEST(MM6VisitedReturnPreEventProbeForTest, RequiresExactTargetAndOneAbort) {
    int calls = 0;
    EXPECT_NO_THROW(mm6::testing::runArmedVisitedReturnPreEventProbeForTest(
        GameTitle::MM6, MAP_INVALID));
    EXPECT_THROW((void) mm6::testing::ScopedVisitedReturnPreEventProbeForTest(
        GameTitle::MM7, MAP_INVALID, [&] { ++calls; }), std::exception);
    {
        mm6::testing::ScopedVisitedReturnPreEventProbeForTest scope(
            GameTitle::MM6, MAP_EMERALD_ISLAND, [&] { ++calls; });
        EXPECT_THROW(mm6::testing::runArmedVisitedReturnPreEventProbeForTest(
            GameTitle::MM6, MAP_INVALID), std::exception);
        EXPECT_EQ(calls, 0);
        EXPECT_FALSE(scope.reached());
    }
    {
        mm6::testing::ScopedVisitedReturnPreEventProbeForTest scope(
            GameTitle::MM6, MAP_EMERALD_ISLAND, [&] { ++calls; });
        EXPECT_THROW(mm6::testing::runArmedVisitedReturnPreEventProbeForTest(
            GameTitle::MM6, MAP_EMERALD_ISLAND),
            mm6::testing::VisitedReturnProbeAbortForTest);
        EXPECT_EQ(calls, 1);
        EXPECT_TRUE(scope.reached());
        EXPECT_THROW(mm6::testing::runArmedVisitedReturnPreEventProbeForTest(
            GameTitle::MM6, MAP_EMERALD_ISLAND), std::exception);
        EXPECT_EQ(calls, 1);
    }
}
