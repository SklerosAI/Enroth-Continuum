#pragma once

#include <stdexcept>
#include <utility>

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Engine/Spells/SpellEnums.h"

#include "Core/Time/Duration.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

// MM6 spell IDs whose shared SpellId names a different MM7 spell or none.
inline constexpr SpellId SPELL_FLAME_ARROW = static_cast<SpellId>(2);
inline constexpr SpellId SPELL_FIRE_BOLT = static_cast<SpellId>(4);
inline constexpr SpellId SPELL_STATIC_CHARGE = static_cast<SpellId>(13);
inline constexpr SpellId SPELL_SPARKS = static_cast<SpellId>(15);
inline constexpr SpellId SPELL_LIGHTNING_BOLT = static_cast<SpellId>(18);
inline constexpr SpellId SPELL_ICE_BOLT = static_cast<SpellId>(28);
inline constexpr SpellId SPELL_MAGIC_ARROW = static_cast<SpellId>(35);
inline constexpr SpellId SPELL_SPIRIT_ARROW = static_cast<SpellId>(45);
inline constexpr SpellId SPELL_FEATHER_FALL = static_cast<SpellId>(16);
inline constexpr SpellId SPELL_JUMP = static_cast<SpellId>(19);
inline constexpr SpellId SPELL_FLY = static_cast<SpellId>(21);
inline constexpr SpellId SPELL_WATER_WALK = static_cast<SpellId>(27);
inline constexpr SpellId SPELL_ENCHANT_ITEM = static_cast<SpellId>(29);
inline constexpr SpellId SPELL_PROTECTION_FROM_MAGIC = static_cast<SpellId>(36);
inline constexpr SpellId SPELL_BLESS = static_cast<SpellId>(46);
inline constexpr SpellId SPELL_HEALING_TOUCH = static_cast<SpellId>(47);
inline constexpr SpellId SPELL_HEROISM = static_cast<SpellId>(51);
inline constexpr SpellId SPELL_CURE_WOUNDS = static_cast<SpellId>(71);
inline constexpr SpellId SPELL_GOLDEN_TOUCH = static_cast<SpellId>(79);

/**
 * @param spell                         MM6 spell ID.
 * @return                              Whether this is one of the MM6 attack spells that launch their own projectile
 *                                      at a monster and whose damage `playerSpellDamage` rolls.
 */
inline bool isPlayerAttackSpell(SpellId spell) {
    return spell == SPELL_FLAME_ARROW || spell == SPELL_FIRE_BOLT || spell == SPELL_STATIC_CHARGE || spell == SPELL_SPARKS ||
           spell == SPELL_LIGHTNING_BOLT || spell == SPELL_ICE_BOLT || spell == SPELL_MAGIC_ARROW || spell == SPELL_SPIRIT_ARROW;
}

/**
 * @offset 0x422000
 * @param spell                         MM6 spell ID.
 * @return                              Whether the spell asks for an item in the caster's backpack, as Enchant Item
 *                                      and Golden Touch do.
 */
inline bool isItemTargetSpell(SpellId spell) {
    return spell == SPELL_ENCHANT_ITEM || spell == SPELL_GOLDEN_TOUCH;
}

/**
 * @param spell                         MM6 spell ID.
 * @return                              Whether a hit by this spell rolls to hit against the monster's armor class, as
 *                                      arrows do. The other attack spells always hit.
 */
inline bool playerSpellRollsToHit(SpellId spell) {
    return spell == SPELL_FLAME_ARROW || spell == SPELL_MAGIC_ARROW || spell == SPELL_SPIRIT_ARROW;
}

/**
 * @param object                        Object of an MM6 player attack spell in flight.
 * @return                              Whether it hits and vanishes like an arrow instead of turning into its impact
 *                                      object. These are the Flame Arrow, Magic Arrow and Spirit Arrow objects.
 */
inline bool isArrowObject(SpriteId object) {
    const int id = std::to_underlying(object);
    return id == 1010 || id == 4010 || id == 5000;
}

/**
 * Damage of one hit by an MM6 player attack spell on a monster, before the monster's resistance.
 *
 * @param spell                         Attack spell, see `isPlayerAttackSpell`.
 * @param spellLevel                    Caster's spell level.
 * @param rng                           Game RNG.
 * @return                              Rolled damage.
 */
inline int playerSpellDamage(SpellId spell, int spellLevel, RandomEngine &rng) {
    if (spell == SPELL_FLAME_ARROW) return rng.randomDice(1, 8);
    if (spell == SPELL_FIRE_BOLT) return rng.randomDice(spellLevel, 4);
    if (spell == SPELL_STATIC_CHARGE) return 2 + rng.random(5);
    if (spell == SPELL_SPARKS) return spellLevel + 2;
    if (spell == SPELL_LIGHTNING_BOLT) return rng.randomDice(spellLevel, 8);
    if (spell == SPELL_ICE_BOLT) return rng.randomDice(spellLevel, 7);
    if (spell == SPELL_MAGIC_ARROW) return 3 + rng.random(6);
    if (spell == SPELL_SPIRIT_ARROW) return rng.randomDice(1, 6);
    throw std::runtime_error("MM6 player spell damage is not verified");
}

/**
 * @param spell                         MM6 spell ID.
 * @return                              Whether the MM6 player cast of this spell runs in the MM6 branch of `castSpell`
 *                                      instead of the shared MM7 case with the same ID.
 */
inline bool hasOwnPlayerCast(SpellId spell) {
    return spell == SPELL_FEATHER_FALL || spell == SPELL_JUMP || spell == SPELL_FLY || isPlayerAttackSpell(spell) ||
           spell == SPELL_PROTECTION_FROM_MAGIC || spell == SPELL_BLESS || spell == SPELL_HEALING_TOUCH ||
           spell == SPELL_HEROISM || spell == SPELL_CURE_WOUNDS;
}

/**
 * @param mastery                       Caster's mastery, Normal to Master.
 * @return                              1, 2 or 3, the multiplier MM6 spells apply to the skill level by mastery.
 */
inline int masteryRank(Mastery mastery) {
    switch (mastery) {
        case MASTERY_NOVICE: return 1;
        case MASTERY_EXPERT: return 2;
        case MASTERY_MASTER: return 3;
        default: break;
    }
    throw std::runtime_error("MM6 has no Grandmaster rank");
}

/**
 * @param mastery                       Caster's mastery, Normal to Master.
 * @param spellLevel                    Caster's spell level.
 * @return                              How long MM6 Fly, Water Walk and Feather Fall last: 5 minutes per level at
 *                                      Normal, 10 at Expert and an hour at Master.
 */
inline Duration travelBuffDuration(Mastery mastery, int spellLevel) {
    switch (masteryRank(mastery)) {
        case 1: return Duration::fromMinutes(5 * spellLevel);
        case 2: return Duration::fromMinutes(10 * spellLevel);
        default: return Duration::fromHours(spellLevel);
    }
}

/**
 * @param mastery                       Caster's mastery, Normal to Master.
 * @param spellLevel                    Caster's spell level.
 * @return                              How long MM6 Bless and Heroism last: 64 minutes plus 5 minutes per level, or 15
 *                                      at Master.
 */
inline Duration blessAndHeroismDuration(Mastery mastery, int spellLevel) {
    return Duration::fromMinutes(64 + (masteryRank(mastery) == 3 ? 15 : 5) * spellLevel);
}

/**
 * @param mastery                       Caster's mastery, Normal to Master.
 * @param rng                           Game RNG.
 * @return                              Hit points that MM6 Healing Touch restores, 2d3 plus 1, 3 or 5 by mastery.
 */
inline int healingTouchAmount(Mastery mastery, RandomEngine &rng) {
    return rng.randomDice(2, 3) + 2 * masteryRank(mastery) - 1;
}

}  // namespace mm6
