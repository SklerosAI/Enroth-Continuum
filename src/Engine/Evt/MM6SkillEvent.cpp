#include "Engine/Evt/MM6SkillEvent.h"

#include <stdexcept>

#include "Engine/Objects/Character.h"
#include "Engine/Objects/CombinedSkillValue.h"

namespace mm6 {

bool isSkillVariable(int rawId) {
    return rawId >= 56 && rawId <= 86;
}

Skill checkedEventSkill(EvtOpcode opcode, int rawId, int value, GameTitle title) {
    if (title != GameTitle::MM6 || !isSkillVariable(rawId))
        throw std::runtime_error("MM6 skill event has a foreign title or variable");
    if (opcode != EVENT_Compare && opcode != EVENT_Set)
        throw std::runtime_error("MM6 skill event operation is not proved by active scripts");

    // The retained active comparisons use plain levels, and the sole Set
    // grants Novice level one. Mastery comparisons retain the shared joined
    // ordering, but MM6 has no Grandmaster skill state.
    if (value < 0 || value > 0xff || (value & 0xc0) == 0xc0)
        throw std::runtime_error("MM6 skill event operand is outside the title range");
    if (opcode == EVENT_Set) {
        auto [level, mastery] = CombinedSkillValue::fromJoinedUnchecked(value);
        if (!CombinedSkillValue::isValid(level, mastery) || mastery > MASTERY_MASTER)
            throw std::runtime_error("MM6 skill event Set has an invalid level or mastery");
    }

    const int sourceIndex = rawId - 56;
    return sourceIndex == 30 ? SKILL_LEARNING : static_cast<Skill>(sourceIndex);
}

bool compareEventSkill(const Character &character, int rawId, int value, GameTitle title) {
    Skill skill = checkedEventSkill(EVENT_Compare, rawId, value, title);
    CombinedSkillValue current = character.getSkillValue(skill);
    return value <= 63 ? current.level() >= value : current.joined() >= value;
}

void setEventSkill(Character &character, int rawId, int value, GameTitle title) {
    Skill skill = checkedEventSkill(EVENT_Set, rawId, value, title);
    character.setSkillValue(skill, CombinedSkillValue::fromJoined(value));
}

}  // namespace mm6
