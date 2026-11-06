#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestTrapDecision.h"

namespace {

mm6::ChestRecord placedTrappedRecord() {
    mm6::ChestRecord record{};
    mm6::writeChest16(record, 2, 3);
    return record;
}

}  // namespace

UNIT_TEST(MM6ChestTrapDecision, ZeroScoreTriggersWithoutDisarmDraw) {
    const auto record = placedTrappedRecord();
    const auto decision = mm6::stageChestTrapDecision(GameTitle::MM6, record, true, 0, 0, 8);
    ASSERT_TRUE(decision);
    EXPECT_FALSE(decision->disarmed);
    EXPECT_EQ(decision->variant, 0);
    EXPECT_EQ(decision->drawsConsumed, 1);
    EXPECT_EQ(decision->nextLcgState, 4243115u);
    EXPECT_EQ(mm6::readChest16(record, 2), 3u);
}

UNIT_TEST(MM6ChestTrapDecision, ThresholdAndTriggeredVariantFollowSourceOrder) {
    const auto record = placedTrappedRecord();
    // Seed 8 first draws 64: 64 % 10 = 4. Equality triggers the trap.
    const auto triggered = mm6::stageChestTrapDecision(GameTitle::MM6, record, true, 6, 2, 8);
    ASSERT_TRUE(triggered);
    EXPECT_FALSE(triggered->disarmed);
    EXPECT_EQ(triggered->variant, 2);  // Second draw is 28170 % 4.
    EXPECT_EQ(triggered->drawsConsumed, 2);
    EXPECT_EQ(triggered->nextLcgState, 1846202050u);

    const auto disarmed = mm6::stageChestTrapDecision(GameTitle::MM6, record, true, 7, 2, 8);
    ASSERT_TRUE(disarmed);
    EXPECT_TRUE(disarmed->disarmed);
    EXPECT_EQ(disarmed->variant, -1);
    EXPECT_EQ(disarmed->drawsConsumed, 1);
    EXPECT_EQ(disarmed->nextLcgState, 4243115u);
}

UNIT_TEST(MM6ChestTrapDecision, RejectsForeignAndUnreadyInputsWithoutMutation) {
    auto record = placedTrappedRecord();
    const auto original = record;
    EXPECT_FALSE(mm6::stageChestTrapDecision(GameTitle::MM7, record, true, 5, 1, 8));
    EXPECT_FALSE(mm6::stageChestTrapDecision(GameTitle::MM6, record, false, 5, 1, 8));
    EXPECT_FALSE(mm6::stageChestTrapDecision(GameTitle::MM6, record, true, -1, 1, 8));
    EXPECT_FALSE(mm6::stageChestTrapDecision(GameTitle::MM6, record, true, 5, 256, 8));
    mm6::writeChest16(record, 2, 1);  // Placement must already have happened.
    EXPECT_FALSE(mm6::stageChestTrapDecision(GameTitle::MM6, record, true, 5, 1, 8));
    EXPECT_EQ(original[2], 3);
}
