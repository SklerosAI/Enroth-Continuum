#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6ConditionVariables.h"
#include "Engine/Objects/CharacterConditions.h"

UNIT_TEST(MM6ConditionVariables, ShippedConditionIdsMapAroundZombieGap) {
    EXPECT_FALSE(mm6::isConditionVariable(86));
    EXPECT_TRUE(mm6::isConditionVariable(87));
    EXPECT_TRUE(mm6::isConditionVariable(104));
    EXPECT_FALSE(mm6::isConditionVariable(105));

    EXPECT_EQ(mm6::checkedCondition(EVENT_Set, 87, 0), CONDITION_CURSED);
    EXPECT_EQ(mm6::checkedCondition(EVENT_Add, 90, 10), CONDITION_FEAR);
    EXPECT_EQ(mm6::checkedCondition(EVENT_Subtract, 90, 10), CONDITION_FEAR);
    EXPECT_EQ(mm6::checkedCondition(EVENT_Set, 104, 0), CONDITION_GOOD);
    EXPECT_NE(mm6::checkedCondition(EVENT_Set, 104, 0), CONDITION_ZOMBIE);
}

UNIT_TEST(MM6ConditionVariables, CompareUsesSelectedCharacterConditionClock) {
    CharacterConditions first;
    CharacterConditions second;
    second.set(CONDITION_CURSED, Time::fromHours(2));
    EXPECT_FALSE(mm6::compareCondition(first, 87, 0));
    EXPECT_TRUE(mm6::compareCondition(second, 87, 0));
    EXPECT_TRUE(mm6::compareCondition(second, 87, 1));
    first.set(CONDITION_FEAR, Time::fromHours(3));
    EXPECT_TRUE(mm6::compareCondition(first, 90, 10));
    EXPECT_FALSE(mm6::compareCondition(second, 90, 10));
}

UNIT_TEST(MM6ConditionVariables, UnprovedOperandsAndOperationsFailClosed) {
    EXPECT_THROW(mm6::checkedCondition(EVENT_Set, 86, 0), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Set, 105, 0), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Add, 104, 0), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Add, 95, 3), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Set, 103, 0), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Subtract, 101, 0), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Set, 95, 3), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Set, 87, 1), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_Compare, 87, 2), std::runtime_error);
    EXPECT_THROW(mm6::checkedCondition(EVENT_OnCanShowDialogItemCmp, 87, 0), std::runtime_error);
}
