#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpellLearning.h"

UNIT_TEST(MM6SpellLearning, UsesSchoolAvailabilityAcrossClassesAndSpells) {
    mm6::CreationRules rules;
    for (int family = 0; family < mm6::kClassFamilyCount; ++family)
        rules.startingSkills[family][12 + family] = family % 3 + 1;

    for (int classId = 0; classId < mm6::kClassCount; ++classId) {
        for (int spellId = 1; spellId <= 99; ++spellId) {
            const bool expected = (spellId - 1) / 11 == classId / mm6::kClassTiers;
            const SpellId spell = static_cast<SpellId>(spellId);
            EXPECT_EQ(mm6::classCanLearnSpellSchool(classId, spell, rules), expected);
            EXPECT_EQ(mm6::hasKnownSpellLearningPrerequisites(classId, spell, true, rules), expected);
            EXPECT_FALSE(mm6::hasKnownSpellLearningPrerequisites(classId, spell, false, rules));
        }
    }
}

UNIT_TEST(MM6SpellLearning, RejectsInvalidClassSpellAndMatrixCode) {
    mm6::CreationRules rules;
    EXPECT_THROW((void) mm6::classCanLearnSpellSchool(-1, static_cast<SpellId>(1), rules), std::out_of_range);
    EXPECT_THROW((void) mm6::classCanLearnSpellSchool(18, static_cast<SpellId>(1), rules), std::out_of_range);
    EXPECT_THROW((void) mm6::classCanLearnSpellSchool(0, SPELL_NONE, rules), std::out_of_range);
    EXPECT_THROW((void) mm6::classCanLearnSpellSchool(0, static_cast<SpellId>(100), rules), std::out_of_range);

    rules.startingSkills[0][12] = 4;
    EXPECT_THROW((void) mm6::classCanLearnSpellSchool(0, static_cast<SpellId>(1), rules), std::runtime_error);
}
