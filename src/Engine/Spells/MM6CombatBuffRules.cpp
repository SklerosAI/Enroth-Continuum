#include "Engine/Spells/MM6CombatBuffRules.h"

#include <stdexcept>
#include <utility>

#include "Engine/MM6/MM6PlayerSpell.h"

namespace mm6 {

bool isCombatBuffSpell(SpellId spellId) {
    return spellId == SPELL_AIR_SHIELD || spellId == SPELL_SPIRIT_HEROISM;
}

CombatBuffPlan planCombatBuff(SpellId spellId, Mastery mastery, int skillLevel, Time now) {
    const int rank = std::to_underlying(mastery);
    if (!isCombatBuffSpell(spellId) || rank < 1 || rank > 3 || skillLevel < 1 || skillLevel > 63 ||
        !now.isValid())
        throw std::runtime_error("Invalid MM6 combat buff spell, mastery, skill or clock");
    CombatBuffPlan result;
    result.buff = spellId == SPELL_AIR_SHIELD ? CHARACTER_BUFF_SHIELD : CHARACTER_BUFF_HEROISM;
    result.expires = now + blessAndHeroismDuration(mastery, skillLevel); // MM6.exe 0x42472A gives Shield the same durations.
    result.mastery = mastery;
    result.power = spellId == SPELL_AIR_SHIELD ? 0 : 5 + skillLevel; // Heroism, MM6.exe 0x426C21.
    result.partyWide = mastery >= MASTERY_EXPERT;
    return result;
}

bool applyCombatBuff(SpellBuff &slot, const CombatBuffPlan &plan, int casterIndex) {
    if (plan.mastery < MASTERY_NOVICE || plan.mastery > MASTERY_MASTER ||
        (plan.buff != CHARACTER_BUFF_SHIELD && plan.buff != CHARACTER_BUFF_HEROISM) ||
        !plan.expires.isValid() || plan.power < 0 || plan.power > 7300 ||
        plan.partyWide != (plan.mastery >= MASTERY_EXPERT) ||
        (plan.buff == CHARACTER_BUFF_SHIELD && plan.power != 0) ||
        casterIndex < 0 || casterIndex >= 4)
        throw std::runtime_error("Invalid MM6 combat buff plan or caster");
    return slot.Apply(plan.expires, plan.mastery, plan.power, 0, casterIndex);
}

}  // namespace mm6
