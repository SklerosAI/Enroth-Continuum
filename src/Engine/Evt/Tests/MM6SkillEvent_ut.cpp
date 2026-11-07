#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6SkillEvent.h"
#include "Engine/Objects/Character.h"

UNIT_TEST(MM6SkillEvent, MapsActiveSkillsWithoutMM7VariableCollision) {
    EXPECT_EQ(mm6::checkedEventSkill(EVENT_Set, 63, 1, GameTitle::MM6), SKILL_BLASTER);
    EXPECT_EQ(mm6::checkedEventSkill(EVENT_Compare, 82, 10, GameTitle::MM6), SKILL_PERCEPTION);
    EXPECT_EQ(mm6::checkedEventSkill(EVENT_Compare, 86, 1, GameTitle::MM6), SKILL_LEARNING);
    EXPECT_THROW(mm6::checkedEventSkill(EVENT_Compare, 82, 1, GameTitle::MM7), std::runtime_error);
    EXPECT_THROW(mm6::checkedEventSkill(EVENT_Compare, 87, 1, GameTitle::MM6), std::runtime_error);
}

UNIT_TEST(MM6SkillEvent, ComparesLevelAndSetsNoviceWithoutChangingOtherSkills) {
    Character character;
    character.setSkillValue(SKILL_PERCEPTION, CombinedSkillValue::novice(4));
    EXPECT_TRUE(mm6::compareEventSkill(character, 82, 4, GameTitle::MM6));
    EXPECT_FALSE(mm6::compareEventSkill(character, 82, 5, GameTitle::MM6));
    EXPECT_FALSE(mm6::compareEventSkill(character, 82, 0x40, GameTitle::MM6));
    character.setSkillValue(SKILL_PERCEPTION, CombinedSkillValue(4, MASTERY_EXPERT));
    EXPECT_TRUE(mm6::compareEventSkill(character, 82, 0x40, GameTitle::MM6));

    mm6::setEventSkill(character, 63, 1, GameTitle::MM6);
    EXPECT_EQ(character.getSkillValue(SKILL_BLASTER), CombinedSkillValue::novice());
    EXPECT_EQ(character.getSkillValue(SKILL_PERCEPTION), CombinedSkillValue(4, MASTERY_EXPERT));
}

UNIT_TEST(MM6SkillEvent, RefusesUnprovedOperationsAndMalformedValuesBeforeMutation) {
    Character character;
    const CombinedSkillValue before = character.getSkillValue(SKILL_BLASTER);
    EXPECT_THROW(mm6::checkedEventSkill(EVENT_Add, 63, 1, GameTitle::MM6), std::runtime_error);
    EXPECT_THROW(mm6::checkedEventSkill(EVENT_Subtract, 63, 1, GameTitle::MM6), std::runtime_error);
    EXPECT_THROW(mm6::setEventSkill(character, 63, 0x101, GameTitle::MM6), std::runtime_error);
    EXPECT_THROW(mm6::setEventSkill(character, 63, 0xc1, GameTitle::MM6), std::runtime_error);
    EXPECT_THROW(mm6::setEventSkill(character, 63, 0x40, GameTitle::MM6), std::runtime_error);
    EXPECT_EQ(character.getSkillValue(SKILL_BLASTER), before);
}
