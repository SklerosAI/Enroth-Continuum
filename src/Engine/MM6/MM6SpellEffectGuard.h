#pragma once

#include <stdexcept>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6MonsterSpellEffect.h"
#include "Engine/Spells/SpellEnumFunctions.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

enum class SpellEffectSource {
    PlayerQueue,
    Event,
    Monster,
    Damage
};

/**
 * @param spell                         MM6 spell ID.
 * @return                              Whether a player cast of this spell from the spellbook has a verified target,
 *                                      cost, recovery and effect. This is Cure Weakness.
 */
inline bool isVerifiedPlayerSpell(SpellId spell) {
    return static_cast<int>(spell) == 67;
}

/**
 * Refuses an MM6 spell whose target and effect are not verified for the given source, before anything changes.
 * Player bow and blaster projectiles are not spells and keep their queue behavior.
 *
 * @param title                         Running game title, everything passes outside MM6.
 * @param spell                         Spell about to take effect.
 * @param source                        Where the cast comes from.
 */
inline void requireVerifiedSpellEffect(GameTitle title, SpellId spell, SpellEffectSource source) {
    if (title != GameTitle::MM6)
        return;
    if (source == SpellEffectSource::PlayerQueue &&
        (spell == SPELL_NONE || spell == SPELL_BOW_ARROW || spell == SPELL_BLASTER_PROJECTILE))
        return;
    if (source == SpellEffectSource::PlayerQueue && isVerifiedPlayerSpell(spell))
        return;
    if (source == SpellEffectSource::Monster && isVerifiedMonsterSpell(spell))
        return;
    throw std::runtime_error("MM6 spell target and effect dispatch are not implemented");
}

/**
 * Refuses an MM6 player cast that uses a fixed skill instead of the caster's own, as scrolls, NPCs and temples do.
 * Only the spellbook cost and recovery rule is verified.
 *
 * @param spell                         Spell being queued.
 * @param hasSkillOverride              Whether the cast supplies its own skill.
 */
inline void requireVerifiedPlayerCastSkill(SpellId spell, bool hasSkillOverride) {
    if (hasSkillOverride && isRegularSpell(spell))
        throw std::runtime_error("MM6 scroll, NPC and temple casts lack a verified cost and recovery rule");
}

/**
 * @param spell                         Spell the player picked in the spellbook or set as the quick spell.
 * @return                              Whether the MM6 interface should turn the cast down instead of queueing it.
 */
inline bool shouldRefusePlayerSpell(SpellId spell) {
    return isRegularSpell(spell) && !isVerifiedPlayerSpell(spell);
}

}  // namespace mm6
