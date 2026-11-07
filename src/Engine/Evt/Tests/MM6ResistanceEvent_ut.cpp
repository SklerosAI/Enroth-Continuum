#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ResistanceEvent.h"
#include "Engine/Objects/Character.h"

UNIT_TEST(MM6ResistanceEvent, IdsAddressMM6ChannelsNotMM7Ones) {
    // MM6 resistance IDs 46-55 must not land on the MM7 resistance fields that share their numbers.
    Character character;
    mm6::setEventResistance(character, 46, 1);
    mm6::setEventResistance(character, 47, 2);
    mm6::setEventResistance(character, 48, 3);
    mm6::setEventResistance(character, 49, 4);
    mm6::setEventResistance(character, 50, 5);
    mm6::setEventResistance(character, 51, 6);
    mm6::setEventResistance(character, 52, 7);
    mm6::setEventResistance(character, 53, 8);
    mm6::setEventResistance(character, 54, 9);
    mm6::setEventResistance(character, 55, 10);

    EXPECT_EQ(character.sResFireBase, 1);
    EXPECT_EQ(character.sResAirBase, 2);
    EXPECT_EQ(character.sResWaterBase, 3);
    EXPECT_EQ(character.sResBodyBase, 4);
    EXPECT_EQ(character.sResMagicBase, 5);
    EXPECT_EQ(character.sResFireBonus, 6);
    EXPECT_EQ(character.sResAirBonus, 7);
    EXPECT_EQ(character.sResWaterBonus, 8);
    EXPECT_EQ(character.sResBodyBonus, 9);
    EXPECT_EQ(character.sResMagicBonus, 10);
    EXPECT_EQ(character.sResEarthBase, 0);
    EXPECT_EQ(character.sResSpiritBase, 0);
    EXPECT_EQ(character.sResMindBase, 0);
    EXPECT_EQ(character.sResPhysicalBase, 0);
    EXPECT_EQ(character.sResLightBase, 0);
    EXPECT_EQ(character.sResDarkBase, 0);
}

UNIT_TEST(MM6ResistanceEvent, OriginalArithmetic) {
    // MM6 resistance writes: Set keeps the low byte, Add caps at 255, Subtract has no floor.
    Character character;
    mm6::setEventResistance(character, 51, 0x1234);
    EXPECT_EQ(character.sResFireBonus, 0x34);

    mm6::addEventResistance(character, 46, 20);
    mm6::addEventResistance(character, 46, 25);
    EXPECT_EQ(character.sResFireBase, 45);
    mm6::addEventResistance(character, 46, 240);
    EXPECT_EQ(character.sResFireBase, 255);

    mm6::subtractEventResistance(character, 50, 30);
    EXPECT_EQ(character.sResMagicBase, -30);

    EXPECT_TRUE(mm6::compareEventResistance(character, 46, 255));
    EXPECT_FALSE(mm6::compareEventResistance(character, 46, 256));
    EXPECT_TRUE(mm6::compareEventResistance(character, 50, -30));
    EXPECT_FALSE(mm6::compareEventResistance(character, 50, 0));
}

UNIT_TEST(MM6ResistanceEvent, ReactionAndRange) {
    // MM6 resistance writes play the stat-increase reaction, the base one for 46-50 and the bonus one for 51-55.
    EXPECT_EQ(mm6::eventResistanceReaction(46), SPEECH_STAT_BASE_INC);
    EXPECT_EQ(mm6::eventResistanceReaction(50), SPEECH_STAT_BASE_INC);
    EXPECT_EQ(mm6::eventResistanceReaction(51), SPEECH_STAT_BONUS_INC);
    EXPECT_EQ(mm6::eventResistanceReaction(55), SPEECH_STAT_BONUS_INC);

    Character character;
    EXPECT_FALSE(mm6::isResistanceVariable(45));
    EXPECT_FALSE(mm6::isResistanceVariable(56));
    EXPECT_THROW(mm6::setEventResistance(character, 56, 1), std::runtime_error);
    EXPECT_THROW(mm6::eventResistanceReaction(45), std::runtime_error);
}
