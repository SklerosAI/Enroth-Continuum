#include <initializer_list>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpellEffectGuard.h"

UNIT_TEST(MM6SpellEffectGuard, RejectsAllRegularIDsAtEveryEffectEntry) {
    for (int id = 1; id <= 99; ++id) {
        const SpellId spell = static_cast<SpellId>(id);
        for (mm6::SpellEffectSource source : {mm6::SpellEffectSource::PlayerQueue,
                                               mm6::SpellEffectSource::Event,
                                               mm6::SpellEffectSource::Monster,
                                               mm6::SpellEffectSource::Damage}) {
            EXPECT_THROW(mm6::requireVerifiedSpellEffect(GameTitle::MM6, spell, source), std::runtime_error);
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
