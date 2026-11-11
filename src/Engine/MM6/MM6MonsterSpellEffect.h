#pragma once

#include <optional>
#include <stdexcept>

#include "Engine/MM6/MM6SpellDamageTag.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Engine/Spells/SpellEnums.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

/**
 * @param spell                         MM6 spell ID from a monster's spell cell.
 * @return                              Whether a monster cast of this spell has a verified MM6 launch, impact, damage
 *                                      and damage type. These are the projectile spells but Paralyze and Toxic Cloud,
 *                                      plus Meteor Shower, Dispel Magic and Mass Curse.
 */
inline bool isVerifiedMonsterSpell(SpellId spell) {
    switch (static_cast<int>(spell)) {
        case 2: case 4: case 6: case 9: case 11: case 18: case 24: case 26: case 28:
        case 37: case 58: case 65: case 70: case 76: case 80: case 91: case 95:
            return true;
        default:
            return false;
    }
}

/**
 * Dice that MM6 rolls for a monster's spell hit. The original monster damage routine uses its own table here, not
 * the player spell formula: most projectile spells roll a single d2, the rest one d2 per skill point.
 */
struct MonsterSpellDice {
    int rolls = 0;
    int sides = 0;
};

/**
 * @param spell                         Monster spell.
 * @param skillLevel                    Skill level from the monster's spell cell.
 * @return                              Dice rolled for one hit. The monster's first attack bonus is added on top.
 */
inline MonsterSpellDice monsterSpellDice(SpellId spell, int skillLevel) {
    switch (static_cast<int>(spell)) {
        case 2: case 4: case 6: case 9: case 11: case 18: case 26: case 28: case 37: case 65: case 70: case 76: case 90:
            return {1, 2};
        default:
            return {skillLevel, 2};
    }
}

/**
 * @param monster                       Casting monster.
 * @param spell                         Monster spell.
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
        case SpellDamageTag::Physical: return DAMAGE_PHYSICAL;
        case SpellDamageTag::Fire: return DAMAGE_FIRE;
        case SpellDamageTag::Electricity: return DAMAGE_AIR;
        case SpellDamageTag::Cold: return DAMAGE_WATER;
        case SpellDamageTag::Poison: return DAMAGE_EARTH; // The shared runtime keeps MM6 Poison in the Earth channel.
        case SpellDamageTag::Magic: return DAMAGE_MAGIC;
        default: break;
    }
    throw std::runtime_error("MM6 monster spell deals no damage");
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

/**
 * @param spell                         Monster spell that flies as a projectile.
 * @return                              Object the original launches for it.
 */
inline SpriteId monsterSpellObject(SpellId spell) {
    switch (static_cast<int>(spell)) {
        case 2: return static_cast<SpriteId>(1010);
        case 4: return static_cast<SpriteId>(1030);
        case 6: return static_cast<SpriteId>(1050);
        case 9: return static_cast<SpriteId>(1080);
        case 11: return static_cast<SpriteId>(1100);
        case 18: return static_cast<SpriteId>(2060);
        case 24: return static_cast<SpriteId>(3010);
        case 26: return static_cast<SpriteId>(3030);
        case 28: return static_cast<SpriteId>(3050);
        case 37: return static_cast<SpriteId>(4030);
        case 58: return static_cast<SpriteId>(6020);
        case 65: return static_cast<SpriteId>(6090);
        case 70: return static_cast<SpriteId>(7030);
        case 76: return static_cast<SpriteId>(7090);
        case 95: return static_cast<SpriteId>(9060);
        default: break;
    }
    throw std::runtime_error("MM6 monster spell has no verified projectile");
}

/**
 * @param skill                         Skill from a monster's spell cell.
 * @return                              The skill as the original's skill byte: the level, plus 0x40 for Expert and 0x80
 *                                      for Master.
 */
inline int monsterSkillByte(CombinedSkillValue skill) {
    return skill.level() | (skill.mastery() == MASTERY_EXPERT ? 0x40 : 0) | (skill.mastery() == MASTERY_MASTER ? 0x80 : 0);
}

/**
 * @param skill                         Skill from a monster's Meteor Shower cell.
 * @return                              Number of meteors. The original gives Expert the most.
 */
inline int monsterMeteorCount(CombinedSkillValue skill) {
    switch (skill.mastery()) {
        case MASTERY_MASTER: return 12;
        case MASTERY_EXPERT: return 16;
        default: return 8;
    }
}

}  // namespace mm6
