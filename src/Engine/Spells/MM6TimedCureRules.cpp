#include "Engine/Spells/MM6TimedCureRules.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace mm6 {

static constexpr std::array<int, 6> kSpellIds = {40, 49, 57, 60, 64, 67};

static Condition conditionForSpell(SpellId spellId) {
    switch (std::to_underlying(spellId)) {
        case 40: return CONDITION_PETRIFIED;
        case 49: return CONDITION_CURSED;
        case 57: return CONDITION_FEAR;
        case 60: return CONDITION_PARALYZED;
        case 64: return CONDITION_INSANE;
        case 67: return CONDITION_WEAK;
        default: throw std::runtime_error("Unsupported MM6 timed cure spell ID");
    }
}

static int secondsPerSkill(SpellId spellId, Mastery mastery, TimedCureWindow window) {
    // MM6.exe cases 0x426355 (40), 0x426B01 (49), 0x427534 (57), 0x427741 (60), 0x4279FC (64) and 0x427D8C (67).
    const bool tripled = window == TIMED_CURE_WINDOW_ORIGINAL && std::to_underlying(spellId) >= 57;
    switch (mastery) {
        case MASTERY_NOVICE: return 180;
        case MASTERY_EXPERT: return tripled ? 10800 : 3600;
        case MASTERY_MASTER: return tripled ? 259200 : 86400;
        default: throw std::runtime_error("MM6 timed cure has invalid mastery");
    }
}

bool isTimedCureSpell(SpellId spellId) {
    for (int id : kSpellIds)
        if (std::to_underlying(spellId) == id)
            return true;
    return false;
}

bool applyTimedCure(SpellId spellId, Mastery mastery, int skillLevel, Time now, CharacterConditions &conditions,
                    TimedCureWindow window) {
    const Condition condition = conditionForSpell(spellId);
    const int rank = std::to_underlying(mastery);
    if (rank < 1 || rank > 3 || skillLevel < 1 || skillLevel > 63 || !now.isValid())
        throw std::runtime_error("MM6 timed cure has invalid mastery, skill or time");
    if (!conditions.has(condition))
        return false;
    const Time onset = conditions.get(condition);
    if (onset > now)
        throw std::runtime_error("MM6 timed cure condition starts in the future");
    const int seconds = secondsPerSkill(spellId, mastery, window) * skillLevel;
    if (now - onset >= Duration::fromSeconds(seconds))
        return false;
    conditions.reset(condition);
    return true;
}

}  // namespace mm6
