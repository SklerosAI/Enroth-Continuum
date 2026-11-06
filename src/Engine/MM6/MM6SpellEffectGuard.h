#pragma once

#include <stdexcept>

#include "Engine/GameTitle.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

enum class SpellEffectSource {
    PlayerQueue,
    Event,
    Monster,
    Damage
};

// No MM6 regular spell has a verified title-specific target and effect path.
// Player bow/blaster projectiles are non-spell actions and keep their existing
// queue behavior. Other sources require a proven spell effect before mutation.
inline void requireVerifiedSpellEffect(GameTitle title, SpellId spell, SpellEffectSource source) {
    if (title != GameTitle::MM6)
        return;
    if (source == SpellEffectSource::PlayerQueue &&
        (spell == SPELL_NONE || spell == SPELL_BOW_ARROW || spell == SPELL_BLASTER_PROJECTILE))
        return;
    throw std::runtime_error("MM6 spell target and effect dispatch are not implemented");
}

}  // namespace mm6
