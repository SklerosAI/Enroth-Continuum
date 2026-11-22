#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ArtifactBonus.h"
#include "Engine/Spells/MM6CombatBuffRules.h"

UNIT_TEST(MM6CombatBuffRules, TwoBuffsThreeRanksAndPartyScope) {
    // Shield and Heroism last 64 minutes plus 5 minutes per skill point, 15 at Master.
    const Time now = Time::fromDays(10);
    for (SpellId spell : {SPELL_AIR_SHIELD, SPELL_SPIRIT_HEROISM}) {
        EXPECT_TRUE(mm6::isCombatBuffSpell(spell));
        for (int rank = 1; rank <= 3; ++rank) {
            const auto plan = mm6::planCombatBuff(spell, static_cast<Mastery>(rank), 3, now);
            EXPECT_EQ(plan.partyWide, rank >= 2);
            EXPECT_EQ(plan.expires, now + Duration::fromMinutes(64 + (rank == 3 ? 15 : 5) * 3));
            EXPECT_EQ(plan.buff, spell == SPELL_AIR_SHIELD ? CHARACTER_BUFF_SHIELD : CHARACTER_BUFF_HEROISM);
            EXPECT_EQ(plan.power, spell == SPELL_AIR_SHIELD ? 0 : 8);
            SpellBuff slot;
            EXPECT_TRUE(mm6::applyCombatBuff(slot, plan, 2));
            EXPECT_EQ(slot.expireTime, plan.expires);
            EXPECT_EQ(slot.power, plan.power);
            EXPECT_EQ(slot.skillMastery, plan.mastery);
            EXPECT_EQ(slot.caster, 2);
            auto shorter = plan;
            shorter.expires -= Duration::fromTicks(1);
            EXPECT_FALSE(mm6::applyCombatBuff(slot, shorter, 2));
        }
    }
}

UNIT_TEST(MM6CombatBuffRules, RejectsUnsupportedInputsBeforeBuffMutation) {
    const Time now = Time::fromDays(10);
    EXPECT_FALSE(mm6::isCombatBuffSpell(SPELL_SPIRIT_BLESS));
    EXPECT_THROW(mm6::planCombatBuff(SPELL_SPIRIT_BLESS, MASTERY_NOVICE, 1, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NONE, 1, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_GRANDMASTER, 1, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 0, now), std::runtime_error);
    EXPECT_THROW(mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, mm6::kMaxCastSkillLevel + 1, now), std::runtime_error);
    SpellBuff slot;
    auto plan = mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 1, now);
    plan.power = 1;
    EXPECT_THROW(mm6::applyCombatBuff(slot, plan, 0), std::runtime_error);
    EXPECT_FALSE(slot.Active());
    plan = mm6::planCombatBuff(SPELL_AIR_SHIELD, MASTERY_NOVICE, 1, now);
    EXPECT_THROW(mm6::applyCombatBuff(slot, plan, 4), std::runtime_error);
    EXPECT_FALSE(slot.Active());
}
