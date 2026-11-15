#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6EventSpell.h"

#include "Library/Random/SequentialRandomEngine.h"

static int damage(int spell, int skillLevel) {
    SequentialRandomEngine rng; // Rolls 1, 2, 3 and so on, modulo the die.
    return mm6::eventSpellDamage(static_cast<SpellId>(spell), skillLevel, rng);
}

UNIT_TEST(MM6EventSpell, EachSpellRollsItsOwnDamage) {
    // Every MM6 event spell used to refuse and end the run.
    EXPECT_EQ(damage(6, 3), 2 + 3 + 4);              // Fireball, 1-6 per level.
    EXPECT_EQ(damage(13, 3), 2 + 1);                 // Static Charge, 2-6 at any level.
    EXPECT_EQ(damage(15, 3), 5);                     // Sparks, level plus 2.
    EXPECT_EQ(damage(24, 5), 2 + 3);                 // Cold Beam, 2-6 at any level.
    EXPECT_EQ(damage(28, 2), 2 + 3);                 // Ice Bolt, 1-7 per level.
    EXPECT_EQ(damage(32, 3), 12 + 2 + 1 + 2);        // Ice Blast, 12 plus 1-2 per level.
    EXPECT_EQ(damage(41, 3), 2 + 3 + 4);             // Rock Blast, 1-8 per level.
    EXPECT_EQ(damage(90, 6), 25 + 2 + 3 + 4 + 5 + 6 + 7); // Toxic Cloud, 25 plus 1-10 per level.
    EXPECT_EQ(damage(6, 0), 0);
}

UNIT_TEST(MM6EventSpell, OnlyTheLowSixBitsOfTheLevelCount) {
    // The sci-fi Sparks event casts at level 100, which the original rolls as 36.
    EXPECT_EQ(damage(15, 100), 38);
    EXPECT_EQ(damage(6, 65), 2);
    EXPECT_EQ(damage(6, 64), 0);
}

UNIT_TEST(MM6EventSpell, OtherSpellsHaveNoEventDamage) {
    // Events cast Flame Arrow and Magic Arrow as Cold Beam, no shipped event casts Fire Bolt or Lightning Bolt, and the
    // rest hit nobody.
    for (int spell : {2, 4, 12, 18, 35, 46, 83, 91, 98})
        EXPECT_THROW(damage(spell, 1), std::runtime_error) << spell;
}

UNIT_TEST(MM6EventSpell, DamageTagsMapToCharacterChannels) {
    // The original spell table stores the seven event damage kinds, and the character damage routine reads them.
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::Fire), DAMAGE_FIRE);
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::Electricity), DAMAGE_AIR);
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::Cold), DAMAGE_WATER);
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::Poison), DAMAGE_EARTH);
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::Magic), DAMAGE_MAGIC);
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::Physical), DAMAGE_PHYSICAL);
    EXPECT_EQ(mm6::spellDamageType(mm6::SpellDamageTag::None), DAMAGE_PHYSICAL);
}
