#include <exception>
#include <functional>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PreEventCheckpointForTest.h"

UNIT_TEST(MM6PreEventCheckpointForTest, UnarmedAndForeignTitleCannotRunCallback) {
    EXPECT_EQ(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
              mm6::testing::PreEventCheckpointResult::Unarmed);
    EXPECT_EQ(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM7),
              mm6::testing::PreEventCheckpointResult::Unarmed);
    EXPECT_THROW(mm6::testing::ScopedPreEventCheckpointForTest(
        GameTitle::MM7, [] {}), std::exception);
    EXPECT_THROW(mm6::testing::ScopedPreEventCheckpointForTest(
        GameTitle::MM6, std::function<void()>{}), std::exception);
}

UNIT_TEST(MM6PreEventCheckpointForTest, OneShotOrderingAndAbort) {
    int calls = 0;
    {
        mm6::testing::ScopedPreEventCheckpointForTest scope(
            GameTitle::MM6, [&] { ++calls; },
            mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
        EXPECT_THROW(mm6::testing::ScopedPreEventCheckpointForTest(
            GameTitle::MM6, [&] { ++calls; }), std::exception);
        EXPECT_THROW(mm6::testing::notePreEventMapLoadForTest(GameTitle::MM6),
                     std::exception);
        EXPECT_THROW(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM7),
                     std::exception);
        EXPECT_EQ(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
                  mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
        EXPECT_EQ(calls, 1);
        EXPECT_FALSE(scope.reachedMapLoad());
        EXPECT_FALSE(scope.reachedInput());
        EXPECT_THROW(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
                     std::exception);
    }
    EXPECT_EQ(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
              mm6::testing::PreEventCheckpointResult::Unarmed);
}

UNIT_TEST(MM6PreEventCheckpointForTest, ContinueCanReachMapLoadOnce) {
    int calls = 0;
    {
        mm6::testing::ScopedPreEventCheckpointForTest scope(GameTitle::MM6, [&] { ++calls; });
        EXPECT_EQ(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
                  mm6::testing::PreEventCheckpointResult::Continue);
        EXPECT_THROW(mm6::testing::notePreEventMapLoadForTest(GameTitle::MM7),
                     std::exception);
        mm6::testing::notePreEventMapLoadForTest(GameTitle::MM6);
        EXPECT_TRUE(scope.reachedMapLoad());
        mm6::testing::notePreEventInputForTest(GameTitle::MM6);
        EXPECT_TRUE(scope.reachedInput());
        EXPECT_THROW(mm6::testing::notePreEventMapLoadForTest(GameTitle::MM6),
                     std::exception);
    }
    EXPECT_EQ(calls, 1);
}

UNIT_TEST(MM6PreEventCheckpointForTest, ThrownCallbackIsConsumed) {
    {
        mm6::testing::ScopedPreEventCheckpointForTest scope(GameTitle::MM6, [] {
            throw std::runtime_error("synthetic refusal");
        });
        EXPECT_THROW(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
                     std::runtime_error);
        EXPECT_THROW(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
                     std::exception);
    }
    EXPECT_EQ(mm6::testing::runArmedPreEventCheckpointForTest(GameTitle::MM6),
              mm6::testing::PreEventCheckpointResult::Unarmed);
}
