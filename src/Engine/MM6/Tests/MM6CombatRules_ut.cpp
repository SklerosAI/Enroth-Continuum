#include <memory>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CombatRules.h"
#include "Engine/Objects/Character.h"
#include "Engine/Party.h"

#include "Library/Random/SequentialRandomEngine.h"

static void setResistances(Character *character) {
    character->sResFireBase = 1;
    character->sResFireBonus = 2;
    character->sResAirBase = 4;
    character->sResAirBonus = 8;
    character->sResWaterBase = 16;
    character->sResWaterBonus = 32;
    character->sResBodyBase = 64;
    character->sResBodyBonus = 128;
    character->sResMagicBase = 256;
    character->sResMagicBonus = 512;
    character->sResEarthBase = 1000;
    character->sResEarthBonus = 1000;
}

UNIT_TEST(MM6CombatRules, EachChannelReadsItsOwnResistance) {
    // MM6 characters used to refuse Poison, Magic and Energy damage. Each channel reads its own resistance, Magic
    // without the Enchanter, and Energy has none.
    Character character;
    setResistances(&character);
    auto party = std::make_unique<Party>();

    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_FIRE, *party, 0), 3);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_AIR, *party, 0), 12);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_WATER, *party, 0), 48);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_EARTH, *party, 0), 192);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_MAGIC, *party, 0), 768);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_ENERGY, *party, 0), 0);

    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_FIRE, *party, 20), 23);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_AIR, *party, 20), 32);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_WATER, *party, 20), 68);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_EARTH, *party, 20), 212);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_MAGIC, *party, 20), 768);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_ENERGY, *party, 20), 0);

    EXPECT_THROW(mm6::characterResistance(character, DAMAGE_PHYSICAL, *party, 0), std::runtime_error);
    EXPECT_THROW(mm6::characterResistance(character, DAMAGE_MIND, *party, 0), std::runtime_error);
}

UNIT_TEST(MM6CombatRules, PartyProtectionAddsToItsChannel) {
    // Each party protection buff adds its power to one channel, and Energy stays unresisted.
    Character character;
    auto party = std::make_unique<Party>();
    party->pPartyBuffs[PARTY_BUFF_RESIST_FIRE].power = 1;
    party->pPartyBuffs[PARTY_BUFF_RESIST_AIR].power = 2;
    party->pPartyBuffs[PARTY_BUFF_RESIST_WATER].power = 4;
    party->pPartyBuffs[PARTY_BUFF_RESIST_BODY].power = 8;
    party->pPartyBuffs[PARTY_BUFF_PROTECTION_FROM_MAGIC].power = 16;
    party->pPartyBuffs[PARTY_BUFF_RESIST_EARTH].power = 1000;

    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_FIRE, *party, 0), 1);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_AIR, *party, 0), 2);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_WATER, *party, 0), 4);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_EARTH, *party, 0), 8);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_MAGIC, *party, 0), 16);
    EXPECT_EQ(mm6::characterResistance(character, DAMAGE_ENERGY, *party, 0), 0);
}

UNIT_TEST(MM6CombatRules, ResistanceRollHalvesUpToFourTimes) {
    // Each roll at or above 30 out of luck bonus plus resistance plus 30 halves the hit, the first lower roll stops it.
    SequentialRandomEngine rng;

    rng.seed(29);
    EXPECT_EQ(mm6::resistedDamage(100, 0, 30, rng), 6);
    EXPECT_EQ(rng.state(), 33);

    rng.seed(59);
    EXPECT_EQ(mm6::resistedDamage(100, 1, 30, rng), 50);
    EXPECT_EQ(rng.state(), 61);

    rng.seed(28);
    EXPECT_EQ(mm6::resistedDamage(100, 0, 30, rng), 100);
    EXPECT_EQ(rng.state(), 29);
}

UNIT_TEST(MM6CombatRules, ResistanceRollRunsWithoutLuckOrResistance) {
    // With nothing to add the roll can't reach 30, but it still draws once. A negative total skips the roll.
    SequentialRandomEngine rng;

    rng.seed(0);
    EXPECT_EQ(mm6::resistedDamage(100, 0, 0, rng), 100);
    EXPECT_EQ(rng.state(), 1);

    EXPECT_EQ(mm6::resistedDamage(100, 0, -30, rng), 100);
    EXPECT_EQ(rng.state(), 1);
}
