#pragma once

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/Objects/MonsterEnums.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

/**
 * @param name                          Special attack cell of a monster in MM6 `monsters.txt`, without the `x<level>`
 *                                      suffix.
 * @return                              Special attack the original reads from it: the first of its 23 names that the
 *                                      lowercased cell contains, none if it contains none of them.
 */
inline MonsterSpecialAttack parseMonsterSpecialAttackName(std::string_view name) {
    constexpr std::array<std::pair<std::string_view, MonsterSpecialAttack>, 23> names = {{
        {"curse", SPECIAL_ATTACK_CURSE},
        {"weak", SPECIAL_ATTACK_WEAK},
        {"asleep", SPECIAL_ATTACK_SLEEP},
        {"affraid", SPECIAL_ATTACK_FEAR},
        {"drunk", SPECIAL_ATTACK_DRUNK},
        {"insane", SPECIAL_ATTACK_INSANE},
        {"poison1", SPECIAL_ATTACK_POISON_WEAK},
        {"poison2", SPECIAL_ATTACK_POISON_MEDIUM},
        {"poison3", SPECIAL_ATTACK_POISON_SEVERE},
        {"disease1", SPECIAL_ATTACK_DISEASE_WEAK},
        {"disease2", SPECIAL_ATTACK_DISEASE_MEDIUM},
        {"disease3", SPECIAL_ATTACK_DISEASE_SEVERE},
        {"paralyze", SPECIAL_ATTACK_PARALYZED},
        {"uncon", SPECIAL_ATTACK_UNCONSCIOUS},
        {"dead", SPECIAL_ATTACK_DEAD},
        {"stone", SPECIAL_ATTACK_PETRIFIED},
        {"errad", SPECIAL_ATTACK_ERADICATED},
        {"brkitem", SPECIAL_ATTACK_BREAK_ANY},
        {"brkarmor", SPECIAL_ATTACK_BREAK_ARMOR},
        {"brkweapon", SPECIAL_ATTACK_BREAK_WEAPON},
        {"steal", SPECIAL_ATTACK_STEAL},
        {"age", SPECIAL_ATTACK_AGING},
        {"drainsp", SPECIAL_ATTACK_MANA_DRAIN},
    }};
    const std::string lowered = ascii::toLower(name);
    for (const auto &[text, type] : names)
        if (lowered.contains(text))
            return type;
    return SPECIAL_ATTACK_NONE;
}

}  // namespace mm6
