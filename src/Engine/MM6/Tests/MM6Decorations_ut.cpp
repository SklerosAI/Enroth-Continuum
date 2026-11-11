#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Decorations.h"

#include "Library/Random/SequentialRandomEngine.h"

static int eventForRoll(int id, int roll, SequentialRandomEngine &rng) {
    rng.seed(roll - 1);
    return mm6::decorationGlobalEvent(static_cast<DecorationId>(id), rng);
}

UNIT_TEST(MM6Decorations, OnlyMM6InteractiveRowsRunGlobalEvents) {
    // MM6 used MM7's interactive rows, which in MM6 are trees and other scenery.
    for (int id : {4, 5, 6, 11, 13, 14, 24, 34})
        EXPECT_FALSE(mm6::isInteractiveDecoration(static_cast<DecorationId>(id)));
    for (int id : {118, 119, 120, 121, 146, 154, 155, 158, 162, 163, 164, 166, 167, 182})
        EXPECT_TRUE(mm6::isInteractiveDecoration(static_cast<DecorationId>(id)));
}

UNIT_TEST(MM6Decorations, GlobalEventFollowsTheRoll) {
    // Each row splits one roll out of 100 into its events, and the crystals roll again out of 10 on 50 or more.
    SequentialRandomEngine rng;

    EXPECT_EQ(eventForRoll(163, 29, rng), 10);
    EXPECT_EQ(eventForRoll(163, 30, rng), 11);
    EXPECT_EQ(eventForRoll(164, 50, rng), 13);
    EXPECT_EQ(eventForRoll(164, 89, rng), 16);
    EXPECT_EQ(eventForRoll(164, 90, rng), 17);
    EXPECT_EQ(eventForRoll(182, 19, rng), 19);
    EXPECT_EQ(eventForRoll(182, 20, rng), 23);
    EXPECT_EQ(eventForRoll(167, 97, rng), 28);
    EXPECT_EQ(eventForRoll(162, 0, rng), 35);

    EXPECT_EQ(eventForRoll(118, 49, rng), 47);
    EXPECT_EQ(rng.state(), 49);
    EXPECT_EQ(eventForRoll(118, 50, rng), 49);
    EXPECT_EQ(rng.state(), 51);

    EXPECT_EQ(eventForRoll(13, 0, rng), 0);
    EXPECT_EQ(rng.state(), 0);
}
