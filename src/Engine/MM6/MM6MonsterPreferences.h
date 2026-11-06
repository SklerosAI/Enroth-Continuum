#pragma once

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/Objects/MonsterEnums.h"

namespace mm6 {

// The same source letter means male in MM6 and Monk in later titles.
inline MonsterAttackPreference mAttackPreference(GameTitle gameTitle) {
    return gameTitle == GameTitle::MM6 ? ATTACK_PREFERENCE_MALE : ATTACK_PREFERENCE_MONK;
}

// MM6 has six three-tier class families; MM7's four-ID family mask aliases them.
inline bool matchesClassAttackPreference(int classId, MonsterAttackPreference preference,
                                         bool includePromotions) {
    if (!isValidClass(classId))
        return false;
    const int comparisonId = includePromotions ? classFamily(classId) * kClassTiers : classId;
    switch (preference) {
        case ATTACK_PREFERENCE_KNIGHT: return comparisonId == 0;
        case ATTACK_PREFERENCE_CLERIC: return comparisonId == 3;
        case ATTACK_PREFERENCE_SORCERER: return comparisonId == 6;
        case ATTACK_PREFERENCE_PALADIN: return comparisonId == 9;
        case ATTACK_PREFERENCE_ARCHER: return comparisonId == 12;
        case ATTACK_PREFERENCE_DRUID: return comparisonId == 15;
        default: return false;
    }
}

}  // namespace mm6
