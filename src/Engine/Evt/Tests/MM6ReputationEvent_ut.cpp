#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ReputationEvent.h"

static bool nobodyHired(NpcProfession) {
    return false;
}

static bool bardAndPirateHired(NpcProfession profession) {
    return profession == NPC_PROFESSION_BARD || profession == NPC_PROFESSION_PIRATE;
}

static bool fourRoguesHired(NpcProfession profession) {
    return profession == NPC_PROFESSION_PIRATE || profession == NPC_PROFESSION_GYPSY ||
           profession == NPC_PROFESSION_DUPER || profession == NPC_PROFESSION_BURGLAR;
}

static bool scholarHired(NpcProfession profession) {
    return profession == NPC_PROFESSION_SCHOLAR;
}

UNIT_TEST(MM6ReputationEvent, AddAndSubtractUseTheOperandLowByte) {
    // MM6 variable 215 writes keep only the operand's low byte, so the shipped Subtract 500 and 1000 take away less.
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Add, 0, 200), 200);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Add, -150, 50), -100);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Subtract, 100, 200), -100);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Subtract, 0, 500), -244);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Subtract, 0, 1000), -232);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Add, 0, 256), 0);
}

UNIT_TEST(MM6ReputationEvent, SetReplacesTheStoredValue) {
    // Set ignores the old reputation and also keeps only the operand's low byte.
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Set, -700, 50), 50);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Set, 300, 0), 0);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Set, 0, 300), 44);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Set, 0, -1), 255);
}

UNIT_TEST(MM6ReputationEvent, WritesAreNotClamped) {
    // Event writes aren't held to the -1500..1500 range that MM6 reputation decay clamps to.
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Add, 1450, 200), 1650);
    EXPECT_EQ(mm6::reputationAfterEventWrite(EVENT_Subtract, -1450, 200), -1650);
}

UNIT_TEST(MM6ReputationEvent, WriteRejectsOtherOpcodes) {
    // Only Set, Add and Subtract write MM6 reputation.
    EXPECT_THROW(mm6::reputationAfterEventWrite(EVENT_Compare, 0, 50), std::runtime_error);
}

UNIT_TEST(MM6ReputationEvent, CompareIsEqualityAndPassesNegativeOperands) {
    // MM6 variable 215 compares for equality, and a negative operand passes whatever the reputation is.
    EXPECT_TRUE(mm6::compareReputation(200, 200));
    EXPECT_FALSE(mm6::compareReputation(250, 200));
    EXPECT_FALSE(mm6::compareReputation(150, 200));
    EXPECT_TRUE(mm6::compareReputation(0, 0));
    EXPECT_TRUE(mm6::compareReputation(-50, -50));
    EXPECT_TRUE(mm6::compareReputation(700, -1));
    EXPECT_TRUE(mm6::compareReputation(-700, -1000));
}

UNIT_TEST(MM6ReputationEvent, HiredProfessionsShiftComparedReputation) {
    // A hired Bard raises the compared reputation by 200, and a Pirate, Gypsy, Duper or Burglar each lowers it by 200.
    EXPECT_EQ(mm6::effectiveReputation(100, &nobodyHired), 100);
    EXPECT_EQ(mm6::effectiveReputation(100, &bardAndPirateHired), 100);
    EXPECT_EQ(mm6::effectiveReputation(100, &fourRoguesHired), -700);
    EXPECT_EQ(mm6::effectiveReputation(100, &scholarHired), 100);
}
