#include <exception>
#include <functional>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"

UNIT_TEST(MM6PostWorldAdmissionForTest, UnarmedMM6AndMM7RemainUnchanged) {
    EXPECT_FALSE(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6));
    EXPECT_FALSE(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM7));
}

UNIT_TEST(MM6PostWorldAdmissionForTest, RunsOnceAfterWorldAndBeforeInputStep) {
    bool worldPrepared = false;
    bool inputStarted = false;
    int calls = 0;
    {
        mm6::testing::ScopedPostWorldAdmissionForTest scope(GameTitle::MM6, [&] {
            EXPECT_TRUE(worldPrepared);
            EXPECT_FALSE(inputStarted);
            ++calls;
        });
        EXPECT_THROW(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM7),
                     std::exception);
        worldPrepared = true;
        EXPECT_TRUE(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6));
        EXPECT_THROW(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6),
                     std::exception);
        inputStarted = true;
    }
    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6));
}

UNIT_TEST(MM6PostWorldAdmissionForTest, RefusalConsumesAndStopsBeforeInputStep) {
    bool inputStarted = false;
    int calls = 0;
    {
        mm6::testing::ScopedPostWorldAdmissionForTest scope(GameTitle::MM6, [&] {
            ++calls;
            throw std::runtime_error("synthetic post-world refusal");
        });
        EXPECT_THROW(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6),
                     std::runtime_error);
        EXPECT_FALSE(inputStarted);
        EXPECT_THROW(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6),
                     std::exception);
    }
    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(inputStarted);
}

UNIT_TEST(MM6PostWorldAdmissionForTest, ScopeTeardownClearsUnusedCallback) {
    int calls = 0;
    EXPECT_THROW(mm6::testing::ScopedPostWorldAdmissionForTest(
        GameTitle::MM7, [&] { ++calls; }), std::exception);
    EXPECT_THROW(mm6::testing::ScopedPostWorldAdmissionForTest(
        GameTitle::MM6, std::function<void()>{}), std::exception);
    {
        mm6::testing::ScopedPostWorldAdmissionForTest scope(GameTitle::MM6, [&] {
            ++calls;
        });
        EXPECT_THROW(mm6::testing::ScopedPostWorldAdmissionForTest(
            GameTitle::MM6, [&] { ++calls; }), std::exception);
    }
    EXPECT_FALSE(mm6::testing::runArmedPostWorldAdmissionForTest(GameTitle::MM6));
    EXPECT_EQ(calls, 0);
}
