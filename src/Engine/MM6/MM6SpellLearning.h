#pragma once

#include <stdexcept>
#include <utility>

#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

// Necessary class/school condition from the private F3 creation matrix.
// This does not establish an individual spell's required mastery.
inline bool classCanLearnSpellSchool(int classId, SpellId spell, const CreationRules &rules) {
    const int spellId = std::to_underlying(spell);
    if (!isValidClass(classId) || spellId < 1 || spellId > 99)
        throw std::out_of_range("Invalid MM6 class or spell ID for learning");

    const int sourceSkill = 12 + (spellId - 1) / 11;
    const uint8_t availability = rules.startingSkills[classFamily(classId)][sourceSkill];
    if (availability > 3)
        throw std::runtime_error("Invalid MM6 school availability code");
    return availability != 0;
}

inline bool hasKnownSpellLearningPrerequisites(int classId, SpellId spell, bool schoolSkillTrained,
                                               const CreationRules &rules) {
    return classCanLearnSpellSchool(classId, spell, rules) && schoolSkillTrained;
}

}  // namespace mm6
