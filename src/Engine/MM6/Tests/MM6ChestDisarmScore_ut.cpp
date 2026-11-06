#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestDisarmScore.h"
#include "Engine/MM6/MM6ChestTrapDecision.h"

UNIT_TEST(MM6ChestDisarmScore, MasteryBitsUseOriginalMultipliers) {
    mm6::ChestDisarmScoreInput input;
    input.title = GameTitle::MM6;
    input.encodedSkill = 5;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 10);
    input.encodedSkill = 0x40 | 5;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 15);
    input.encodedSkill = 0x80 | 5;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 20);
    input.encodedSkill = 0xC0 | 63;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 252);
}

UNIT_TEST(MM6ChestDisarmScore, EquipmentDoublesBeforeHirelingBonusesAndMastery) {
    mm6::ChestDisarmScoreInput input;
    input.title = GameTitle::MM6;
    input.encodedSkill = 0x40 | 5;
    input.unbrokenItem410 = true;
    input.unbrokenItem415 = true;
    input.unbrokenThieveryEnchantment = true;
    input.hasTinker = true;
    input.hasLocksmith = true;
    input.hasBurglar = true;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 174);  // (5*8+4+6+8)*3.

    input.encodedSkill = 0;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 36);  // Hirelings alone can give a score.
    mm6::ChestRecord record{};
    mm6::writeChest16(record, 2, 3);
    auto decision = mm6::stageChestTrapDecision(GameTitle::MM6, record, true,
                                               *mm6::stageChestDisarmScore(input), 2, 8);
    ASSERT_TRUE(decision);
    EXPECT_TRUE(decision->disarmed);
    EXPECT_EQ(decision->drawsConsumed, 1);
}

UNIT_TEST(MM6ChestDisarmScore, ForeignTitleRejectsWithoutChangingInput) {
    mm6::ChestDisarmScoreInput input;
    input.title = GameTitle::MM7;
    input.encodedSkill = 0xFF;
    input.unbrokenItem410 = true;
    EXPECT_FALSE(mm6::stageChestDisarmScore(input));
    EXPECT_EQ(input.encodedSkill, 0xFF);
    EXPECT_TRUE(input.unbrokenItem410);
}
