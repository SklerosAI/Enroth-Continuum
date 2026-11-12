#include <initializer_list>
#include <set>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpellEffectGuard.h"

UNIT_TEST(MM6SpellEffectGuard, RejectsUnverifiedRegularIDsAtEveryEffectEntry) {
    // Only the verified attack, travel and support spells are open to player casts, and only the attack spells to hits.
    // Every monster spell but Paralyze and Toxic Cloud is open to monster casts, and every spell that a shipped event
    // casts to event casts.
    const std::set<int> attackSpells = {2, 4, 13, 15, 18, 28, 35, 45};
    const std::set<int> playerSpells = {1, 2, 3, 4, 12, 13, 14, 15, 16, 18, 19, 21, 25, 27, 28, 35, 36, 45, 46, 47, 51, 67, 68, 69, 71};
    const std::set<int> monsterSpells = {2, 4, 6, 9, 11, 18, 24, 26, 28, 37, 58, 65, 70, 76, 80, 91, 95};
    const std::set<int> eventSpells = {2, 6, 12, 13, 15, 24, 28, 32, 35, 41, 46, 83, 90, 91, 98};
    for (int id = 1; id <= 99; ++id) {
        const SpellId spell = static_cast<SpellId>(id);
        for (mm6::SpellEffectSource source : {mm6::SpellEffectSource::PlayerQueue,
                                               mm6::SpellEffectSource::Event,
                                               mm6::SpellEffectSource::Monster,
                                               mm6::SpellEffectSource::Damage}) {
            const bool verified = (source == mm6::SpellEffectSource::PlayerQueue && playerSpells.contains(id)) ||
                                  (source == mm6::SpellEffectSource::Damage && attackSpells.contains(id)) ||
                                  (source == mm6::SpellEffectSource::Monster && monsterSpells.contains(id)) ||
                                  (source == mm6::SpellEffectSource::Event && eventSpells.contains(id));
            if (verified) {
                EXPECT_NO_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, spell, source)) << id;
            } else {
                EXPECT_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, spell, source), std::runtime_error) << id;
            }
            EXPECT_NO_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM7, spell, source));
        }
    }
}

UNIT_TEST(MM6SpellEffectGuard, RetainsOnlyNonSpellPlayerQueueActions) {
    for (SpellId spell : {SPELL_NONE, SPELL_BOW_ARROW, SPELL_BLASTER_PROJECTILE}) {
        EXPECT_NO_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, spell,
                                                        mm6::SpellEffectSource::PlayerQueue));
        EXPECT_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, spell,
                                                     mm6::SpellEffectSource::Event), std::runtime_error);
        EXPECT_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, spell,
                                                     mm6::SpellEffectSource::Monster), std::runtime_error);
    }
    EXPECT_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, SPELL_101,
                                                 mm6::SpellEffectSource::PlayerQueue), std::runtime_error);
}
