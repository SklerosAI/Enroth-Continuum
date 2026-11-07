#pragma once

#include <optional>
#include <stdexcept>

#include "Engine/MM6/MM6SpellDamageTag.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Spells/SpellEnums.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

/**
 * @param spell                         MM6 spell ID from a monster's spell cell.
 * @return                              Whether a monster cast of this spell has a verified MM6 launch, damage and
 *                                      damage type. These are Flame Arrow, Fire Bolt, Lightning Bolt and Cold Beam.
 */
inline bool isVerifiedMonsterSpell(SpellId spell) {
    const int id = static_cast<int>(spell);
    return id == 2 || id == 4 || id == 18 || id == 24;
}

/**
 * Dice that MM6 rolls for a monster's spell hit. The original monster damage routine uses its own table here, not
 * the player spell formula: most projectile spells roll a single d2, a few roll one d2 per skill point.
 */
struct MonsterSpellDice {
    int rolls = 0;
    int sides = 0;
};

/**
 * @param spell                         Verified monster spell, see `isVerifiedMonsterSpell`.
 * @param skillLevel                    Skill level from the monster's spell cell.
 * @return                              Dice rolled for one hit. The monster's first attack bonus is added on top.
 */
inline MonsterSpellDice monsterSpellDice(SpellId spell, int skillLevel) {
    if (!isVerifiedMonsterSpell(spell))
        throw std::runtime_error("MM6 monster spell damage is not verified");
    if (static_cast<int>(spell) == 24)
        return {skillLevel, 2};
    return {1, 2};
}

/**
 * @param monster                       Casting monster.
 * @param spell                         Verified monster spell.
 * @param skill                         Skill from the monster's spell cell.
 * @param rng                           Game RNG.
 * @return                              Damage of one spell hit, before the target's resistance.
 */
inline int monsterSpellDamage(const MonsterInfo &monster, SpellId spell, CombinedSkillValue skill, RandomEngine &rng) {
    const MonsterSpellDice dice = monsterSpellDice(spell, skill.level());
    int damage = 0;
    for (int i = 0; i < dice.rolls; i++)
        damage += rng.random(dice.sides) + 1;
    return damage + monster.attack1DamageBonus;
}

/**
 * @param tag                           Damage tag of the spell in MM6 `spells.txt`.
 * @return                              Damage type of a monster cast. Vanilla MM6 dealt Fire damage for every monster
 *                                      spell, GrayFace's patch uses the spell's own element as done here.
 */
inline DamageType monsterSpellDamageType(SpellDamageTag tag) {
    switch (tag) {
        case SpellDamageTag::Fire: return DAMAGE_FIRE;
        case SpellDamageTag::Electricity: return DAMAGE_AIR;
        case SpellDamageTag::Cold: return DAMAGE_WATER;
        default: break;
    }
    throw std::runtime_error("MM6 monster spell damage type is not verified");
}

/**
 * @param tag                           Damage tag of the spell in MM6 `spells.txt`, if it has one.
 * @return                              Damage type of a monster cast.
 */
inline DamageType monsterSpellDamageType(const std::optional<SpellDamageTag> &tag) {
    if (!tag)
        throw std::runtime_error("MM6 monster spell has no damage tag");
    return monsterSpellDamageType(*tag);
}

}  // namespace mm6
