#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ArtifactBonus.h"
#include "Engine/Spells/MM6PowerCureRules.h"

UNIT_TEST(MM6PowerCureRules, ThreeRanksAndSkillBounds) {
    for (Mastery rank : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
        EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, rank, 1), 12);
        EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, rank, 63), 136);
    }
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_NONE, 1), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_GRANDMASTER, 1), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_NOVICE, 0), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, MASTERY_NOVICE, mm6::kMaxCastSkillLevel + 1), std::runtime_error);
    EXPECT_THROW(mm6::powerCureHealAmount(SPELL_BODY_FIRST_AID, MASTERY_NOVICE, 1), std::runtime_error);
}
