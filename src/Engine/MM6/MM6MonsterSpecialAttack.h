#pragma once

#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

// Exact active MM6 monsters.txt names after the loader removes the x<level>
// suffix. A misspelling in the retained table is an intentional alias.
inline MonsterSpecialAttack parseMonsterSpecialAttackName(std::string_view name) {
    while (!name.empty() && name.back() == ' ')
        name.remove_suffix(1);
    constexpr std::array<std::pair<std::string_view, MonsterSpecialAttack>, 24> names = {{
        {"Curse", SPECIAL_ATTACK_CURSE},
        {"Weak", SPECIAL_ATTACK_WEAK},
        {"Asleep", SPECIAL_ATTACK_SLEEP},
        {"Affraid", SPECIAL_ATTACK_FEAR},
        {"Drunk", SPECIAL_ATTACK_DRUNK},
        {"Insane", SPECIAL_ATTACK_INSANE},
        {"Poison1", SPECIAL_ATTACK_POISON_WEAK},
        {"Poison2", SPECIAL_ATTACK_POISON_MEDIUM},
        {"Poison3", SPECIAL_ATTACK_POISON_SEVERE},
        {"Pois2", SPECIAL_ATTACK_POISON_MEDIUM},
        {"Pois3", SPECIAL_ATTACK_POISON_SEVERE},
        {"Disease1", SPECIAL_ATTACK_DISEASE_WEAK},
        {"Disease2", SPECIAL_ATTACK_DISEASE_MEDIUM},
        {"Disease3", SPECIAL_ATTACK_DISEASE_SEVERE},
        {"Paralyze", SPECIAL_ATTACK_PARALYZED},
        {"Uncon", SPECIAL_ATTACK_UNCONSCIOUS},
        {"Dead", SPECIAL_ATTACK_DEAD},
        {"Stone", SPECIAL_ATTACK_PETRIFIED},
        {"Errad", SPECIAL_ATTACK_ERADICATED},
        {"BrkItem", SPECIAL_ATTACK_BREAK_ANY},
        {"BrkArmor", SPECIAL_ATTACK_BREAK_ARMOR},
        {"BrkWeapon", SPECIAL_ATTACK_BREAK_WEAPON},
        {"Steal", SPECIAL_ATTACK_STEAL},
        {"DrainSP", SPECIAL_ATTACK_MANA_DRAIN},
    }};
    if (ascii::noCaseEquals(name, "Age"))
        return SPECIAL_ATTACK_AGING;
    for (const auto &[text, type] : names)
        if (ascii::noCaseEquals(name, text))
            return type;
    throw std::runtime_error("Unknown active MM6 monster special attack");
}

/**
 * Refuses an MM6 special attack whose save, target choice and effect are not verified. Breaking a random item is the
 * one verified type.
 *
 * @param title                         Running game title, everything passes outside MM6.
 * @param type                          Special attack of the hitting monster.
 */
inline void requireVerifiedMonsterSpecialAttack(GameTitle title, MonsterSpecialAttack type) {
    if (title == GameTitle::MM6 && type != SPECIAL_ATTACK_NONE && type != SPECIAL_ATTACK_BREAK_ANY)
        throw std::runtime_error("MM6 monster special attack effect is unverified");
}

}  // namespace mm6
