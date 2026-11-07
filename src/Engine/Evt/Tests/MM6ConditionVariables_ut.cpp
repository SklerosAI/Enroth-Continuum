#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ConditionVariables.h"
#include "Engine/Objects/CharacterConditions.h"

UNIT_TEST(MM6ConditionVariables, ShippedConditionIdsMapAroundZombieGap) {
    EXPECT_FALSE(mm6::isConditionVariable(86));
    EXPECT_TRUE(mm6::isConditionVariable(87));
    EXPECT_TRUE(mm6::isConditionVariable(104));
    EXPECT_FALSE(mm6::isConditionVariable(105));

    EXPECT_EQ(mm6::eventCondition(87), CONDITION_CURSED);
    EXPECT_EQ(mm6::eventCondition(90), CONDITION_FEAR);
    EXPECT_EQ(mm6::eventCondition(101), CONDITION_DEAD);
    EXPECT_EQ(mm6::eventCondition(103), CONDITION_ERADICATED);
    EXPECT_EQ(mm6::eventCondition(104), CONDITION_GOOD);
    EXPECT_THROW(mm6::eventCondition(86), std::runtime_error);
    EXPECT_THROW(mm6::eventCondition(105), std::runtime_error);
}

UNIT_TEST(MM6ConditionVariables, CompareUsesSelectedCharacterConditionClock) {
    CharacterConditions first;
    CharacterConditions second;
    second.set(CONDITION_CURSED, Time::fromHours(2));
    EXPECT_FALSE(mm6::compareCondition(first, 87));
    EXPECT_TRUE(mm6::compareCondition(second, 87));
    first.set(CONDITION_FEAR, Time::fromHours(3));
    EXPECT_TRUE(mm6::compareCondition(first, 90));
    EXPECT_FALSE(mm6::compareCondition(second, 90));
    EXPECT_THROW(mm6::compareCondition(first, 104), std::runtime_error);
}

UNIT_TEST(MM6ConditionVariables, WritesStampTheClockWithoutSideEffects) {
    // MM6 scripts that kill or stone a character threw instead of setting the condition.
    CharacterConditions conditions;
    mm6::startCondition(conditions, 101, Time::fromHours(5));
    mm6::startCondition(conditions, 102, Time::fromHours(6));
    EXPECT_EQ(conditions.get(CONDITION_DEAD), Time::fromHours(5));
    EXPECT_EQ(conditions.get(CONDITION_PETRIFIED), Time::fromHours(6));

    mm6::startCondition(conditions, 101, Time::fromHours(7));
    EXPECT_EQ(conditions.get(CONDITION_DEAD), Time::fromHours(7));

    EXPECT_TRUE(mm6::clearCondition(conditions, 101));
    EXPECT_FALSE(conditions.has(CONDITION_DEAD));
    EXPECT_TRUE(conditions.has(CONDITION_PETRIFIED));
}

UNIT_TEST(MM6ConditionVariables, GoodClearsOnSetAndIsIgnoredOnSubtract) {
    CharacterConditions conditions;
    conditions.set(CONDITION_WEAK, Time::fromHours(1));
    conditions.set(CONDITION_DISEASE_SEVERE, Time::fromHours(2));
    EXPECT_FALSE(mm6::clearCondition(conditions, 104));
    EXPECT_TRUE(conditions.has(CONDITION_WEAK));

    mm6::startCondition(conditions, 104, Time::fromHours(3));
    EXPECT_FALSE(conditions.has(CONDITION_WEAK));
    EXPECT_FALSE(conditions.has(CONDITION_DISEASE_SEVERE));
}
