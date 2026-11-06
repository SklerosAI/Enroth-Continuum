#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6EventVariables.h"

UNIT_TEST(MM6EventVariables, TitleSpecificRangesNeverUseMM7Dispatch) {
    EXPECT_EQ(describeMM6Variable(45).family, MM6VariableFamily::SharedCandidate);
    EXPECT_TRUE(describeMM6Variable(45).canUseInheritedExecutor());

    for (int rawId = 46; rawId <= 226; ++rawId)
        EXPECT_FALSE(describeMM6Variable(rawId).canUseInheritedExecutor()) << rawId;
    EXPECT_EQ(describeMM6Variable(0).family, MM6VariableFamily::Invalid);
    EXPECT_EQ(describeMM6Variable(227).family, MM6VariableFamily::Invalid);
    EXPECT_FALSE(describeMM6Variable(255).canUseInheritedExecutor());
}

UNIT_TEST(MM6EventVariables, MapByteRangeHasOneHundredCheckedSlots) {
    EXPECT_EQ(describeMM6Variable(104).family, MM6VariableFamily::Condition);
    EXPECT_EQ(describeMM6Variable(105).index, 0);
    EXPECT_EQ(describeMM6Variable(105).scope, MM6VariableScope::Map);
    EXPECT_EQ(describeMM6Variable(204).index, 99);
    EXPECT_EQ(describeMM6Variable(205).family, MM6VariableFamily::OtherTitleSpecific);
}
