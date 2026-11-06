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

UNIT_TEST(MM6ChestDisarmScore, DuplicateEquippedMatchesDoubleOnlyOncePerCategory) {
    std::array<mm6::ChestDisarmEquippedItem, 16> equipped{};
    equipped[0] = {415, 35, false};
    equipped[1] = {415, 35, false};
    equipped[6] = {410, 35, false};
    equipped[7] = {410, 35, false};  // The original checks item 410 only in slot 6.
    auto flags = mm6::scanChestDisarmEquipment(GameTitle::MM6, equipped);
    ASSERT_TRUE(flags);
    mm6::ChestDisarmScoreInput input;
    input.title = GameTitle::MM6;
    input.encodedSkill = 5;
    input.unbrokenItem410 = flags->unbrokenItem410;
    input.unbrokenItem415 = flags->unbrokenItem415;
    input.unbrokenThieveryEnchantment = flags->unbrokenThieveryEnchantment;
    EXPECT_EQ(mm6::stageChestDisarmScore(input), 80);  // 5 * 2 * 2 * 2 * basic 2.

    equipped[6].broken = true;
    equipped[0].broken = true;
    equipped[1].broken = true;
    equipped[7].broken = true;
    flags = mm6::scanChestDisarmEquipment(GameTitle::MM6, equipped);
    ASSERT_TRUE(flags);
    EXPECT_FALSE(flags->unbrokenItem410);
    EXPECT_FALSE(flags->unbrokenItem415);
    EXPECT_FALSE(flags->unbrokenThieveryEnchantment);
    EXPECT_FALSE(mm6::scanChestDisarmEquipment(GameTitle::MM7, equipped));
}
