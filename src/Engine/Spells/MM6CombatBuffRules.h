#pragma once

#include "Core/Time/Time.h"

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellBuff.h"
#include "Engine/Spells/SpellEnums.h"

class Blob;

namespace mm6 {

struct CombatBuffPlan {
    CharacterBuff buff = CHARACTER_BUFF_FIRST;
    Time expires;
    Mastery mastery = MASTERY_NONE;
    int power = 0;
    bool partyWide = false;
};

// Shield and Heroism share target scope and rank duration. The profile holds
// source-checked coefficients; live casting remains blocked by the MM6 guard.
void loadCombatBuffRules(const Blob &source);
bool isCombatBuffSpell(SpellId spellId);
CombatBuffPlan planCombatBuff(SpellId spellId, Mastery mastery, int skillLevel, Time now);

// Writes only the selected buff slot. The caller selects one character at
// Normal rank or every character at Expert/Master rank.
bool applyCombatBuff(SpellBuff &slot, const CombatBuffPlan &plan, int casterIndex);

}  // namespace mm6
