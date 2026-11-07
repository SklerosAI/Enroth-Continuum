#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6StandardBonusMap.h"

UNIT_TEST(MM6StandardBonusMap, AllFourteenSourceIndicesRoundTrip) {
    for (std::uint32_t index = 1; index <= 14; ++index) {
        const auto attribute = mm6::attributeForStandardBonus(index);
        ASSERT_TRUE(attribute);
        EXPECT_EQ(mm6::standardBonusForAttribute(*attribute), index);
    }
    EXPECT_EQ(mm6::attributeForStandardBonus(1), ATTRIBUTE_MIGHT);
    EXPECT_EQ(mm6::attributeForStandardBonus(11), ATTRIBUTE_RESIST_FIRE);
    EXPECT_EQ(mm6::attributeForStandardBonus(12), ATTRIBUTE_RESIST_AIR);
    EXPECT_EQ(mm6::attributeForStandardBonus(13), ATTRIBUTE_RESIST_WATER);
    EXPECT_EQ(mm6::attributeForStandardBonus(14), ATTRIBUTE_RESIST_BODY);
    EXPECT_FALSE(mm6::attributeForStandardBonus(0));
    EXPECT_FALSE(mm6::attributeForStandardBonus(15));
    EXPECT_FALSE(mm6::standardBonusForAttribute(ATTRIBUTE_RESIST_EARTH));
    EXPECT_FALSE(mm6::standardBonusForAttribute(ATTRIBUTE_RESIST_MIND));
}
