#include "Engine/Spells/MM6PowerCureRules.h"

#include <stdexcept>
#include <utility>

#include "Engine/MM6/MM6ArtifactBonus.h"

namespace mm6 {

int powerCureHealAmount(SpellId spellId, Mastery mastery, int skillLevel) {
    const int rank = std::to_underlying(mastery);
    if (spellId != SPELL_BODY_POWER_CURE || rank < 1 || rank > 3 || skillLevel < 1 || skillLevel > kMaxCastSkillLevel)
        throw std::runtime_error("MM6 Power Cure lookup has invalid ID, mastery or skill level");
    return 10 + 2 * skillLevel; // MM6.exe 0x428591, the same at every mastery.
}

}  // namespace mm6
