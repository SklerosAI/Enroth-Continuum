#include "Engine/Spells/MM6FirstAidRules.h"

#include <stdexcept>

namespace mm6 {

int firstAidHealAmount(SpellId spellId, Mastery mastery) {
    if (spellId != SPELL_BODY_FIRST_AID)
        throw std::runtime_error("MM6 First Aid lookup has invalid ID or mastery");
    switch (mastery) { // MM6.exe 0x427E25.
        case MASTERY_NOVICE: return 5;
        case MASTERY_EXPERT: return 7;
        case MASTERY_MASTER: return 10;
        default: throw std::runtime_error("MM6 First Aid lookup has invalid ID or mastery");
    }
}

}  // namespace mm6
