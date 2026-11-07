#pragma once

#include <stdexcept>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6MonsterSpellEffect.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

enum class SpellEffectSource {
    PlayerQueue,
    Event,
    Monster,
    Damage
};

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
    if (source == SpellEffectSource::Monster && isVerifiedMonsterSpell(spell))
        return;
    throw std::runtime_error("MM6 spell target and effect dispatch are not implemented");
}

}  // namespace mm6
