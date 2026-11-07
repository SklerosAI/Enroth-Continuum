#include <initializer_list>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpellEffectGuard.h"

UNIT_TEST(MM6SpellEffectGuard, RejectsUnverifiedRegularIDsAtEveryEffectEntry) {
    // Only the four first-map monster spells are open, and only for monster casts.
    for (int id = 1; id <= 99; ++id) {
        const SpellId spell = static_cast<SpellId>(id);
        for (mm6::SpellEffectSource source : {mm6::SpellEffectSource::PlayerQueue,
                                               mm6::SpellEffectSource::Event,
                                               mm6::SpellEffectSource::Monster,
                                               mm6::SpellEffectSource::Damage}) {
            const bool verified = source == mm6::SpellEffectSource::Monster && (id == 2 || id == 4 || id == 18 || id == 24);
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
