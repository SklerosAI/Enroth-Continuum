#include <exception>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6TransitionAutosaveInterceptForTest.h"

UNIT_TEST(MM6TransitionAutosaveInterceptForTest, UnarmedTitlesKeepNormalAutosave) {
    const auto source = static_cast<MapId>(1);
    const auto target = static_cast<MapId>(2);
    EXPECT_FALSE(mm6::testing::interceptTransitionAutosaveForTest(
        GameTitle::MM6, source, target));
    EXPECT_FALSE(mm6::testing::interceptTransitionAutosaveForTest(
        GameTitle::MM7, source, target));
}

UNIT_TEST(MM6TransitionAutosaveInterceptForTest, ExactPairIsConsumedOnlyOnce) {
    const auto source = static_cast<MapId>(1);
    const auto target = static_cast<MapId>(2);
    {
        mm6::testing::ScopedTransitionAutosaveInterceptForTest scope(
            GameTitle::MM6, source, target);
        EXPECT_THROW(mm6::testing::ScopedTransitionAutosaveInterceptForTest(
            GameTitle::MM6, source, target), std::exception);
        EXPECT_THROW((void) mm6::testing::interceptTransitionAutosaveForTest(
            GameTitle::MM6, target, source), std::exception);
        EXPECT_THROW((void) mm6::testing::interceptTransitionAutosaveForTest(
            GameTitle::MM7, source, target), std::exception);
        EXPECT_FALSE(scope.intercepted());
        EXPECT_TRUE(mm6::testing::interceptTransitionAutosaveForTest(
            GameTitle::MM6, source, target));
        EXPECT_TRUE(scope.intercepted());
        EXPECT_THROW((void) mm6::testing::interceptTransitionAutosaveForTest(
            GameTitle::MM6, source, target), std::exception);
    }
    EXPECT_FALSE(mm6::testing::interceptTransitionAutosaveForTest(
        GameTitle::MM6, source, target));
}

UNIT_TEST(MM6TransitionAutosaveInterceptForTest, RefusesForeignOrSameMapScope) {
    const auto source = static_cast<MapId>(1);
    const auto target = static_cast<MapId>(2);
    EXPECT_THROW(mm6::testing::ScopedTransitionAutosaveInterceptForTest(
        GameTitle::MM7, source, target), std::exception);
    EXPECT_THROW(mm6::testing::ScopedTransitionAutosaveInterceptForTest(
        GameTitle::MM6, source, source), std::exception);
    EXPECT_THROW(mm6::testing::ScopedTransitionAutosaveInterceptForTest(
        GameTitle::MM6, MAP_INVALID, target), std::exception);
}
