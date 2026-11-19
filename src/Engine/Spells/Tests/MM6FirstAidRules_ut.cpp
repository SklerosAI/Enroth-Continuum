#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6FirstAidRules.h"

UNIT_TEST(MM6FirstAidRules, ThreeRanksAndOnlyReviewedSpell) {
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_NOVICE), 5);
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_EXPERT), 7);
    EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_MASTER), 10);
    EXPECT_THROW(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_NONE), std::runtime_error);
    EXPECT_THROW(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, MASTERY_GRANDMASTER), std::runtime_error);
    EXPECT_THROW(mm6::firstAidHealAmount(SPELL_BODY_CURE_WEAKNESS, MASTERY_NOVICE), std::runtime_error);
}
