#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PartyDeath.h"
#include "Engine/Party.h"

static std::unique_ptr<Party> makeDeadParty(Mastery guardianAngel) {
    auto party = std::make_unique<Party>();
    party->playing_time = Time::fromDays(10);
    party->uNumDeaths = 2;
    party->uNumGold = 101;
    party->pos = Vec3f(3, 4, 5);
    party->velocity = Vec3f(6, 7, 8);
    party->_viewYaw = 100;
    party->_viewPitch = 20;
    party->uFlags = PARTY_FLAG_WATER_DAMAGE | PARTY_FLAG_BURNING;
    party->pHirelings[0].name = "hireling";
    party->setActiveCharacterIndex(-1);
    party->pPartyBuffs[static_cast<PartyBuff>(0)].expireTime = Time::fromDays(11);
    if (guardianAngel != MASTERY_NONE) {
        party->pPartyBuffs[static_cast<PartyBuff>(8)].expireTime = Time::fromDays(11);
        party->pPartyBuffs[static_cast<PartyBuff>(8)].skillMastery = guardianAngel;
    }
    for (Character &character : party->pCharacters) {
        character.health = -5;
        character.mana = 9;
        character.conditions.set(CONDITION_DEAD, Time::fromTicks(1));
        character.pCharacterBuffs[static_cast<CharacterBuff>(0)].expireTime = Time::fromDays(11);
    }
    return party;
}

static int maxHealthByIndex(const Party &party, const Character &character) {
    return 21 + 20 * static_cast<int>(&character - party.pCharacters.data());
}

UNIT_TEST(MM6PartyDeath, PartyWakesInNewSorpigalWithNothing) {
    // How the original MM6 handles a dead party without a Guardian Angel.
    auto party = makeDeadParty(MASTERY_NONE);
    mm6::applyPartyDeath(party.get(), [&](const Character &character) { return maxHealthByIndex(*party, character); });

    EXPECT_EQ(party->pos, Vec3f(-9728, -11319, 160));
    EXPECT_EQ(party->uFallStartZ, 160);
    EXPECT_EQ(party->velocity, Vec3f());
    EXPECT_EQ(party->_viewYaw, 512);
    EXPECT_EQ(party->_viewPitch, 0);
    EXPECT_EQ(party->uNumDeaths, 3);
    EXPECT_EQ(party->playing_time, Time::fromDays(17));
    EXPECT_EQ(party->uNumGold, 0);
    EXPECT_EQ(party->uFlags, PARTY_FLAG_BURNING); // The original clears only water damage.
    EXPECT_TRUE(party->pHirelings[0].name.empty());
    EXPECT_FALSE(party->pPartyBuffs[static_cast<PartyBuff>(0)].Active());
    EXPECT_EQ(party->activeCharacterIndex(), 0);
    for (const Character &character : party->pCharacters) {
        EXPECT_EQ(character.health, 1);
        EXPECT_EQ(character.mana, 9);
        EXPECT_FALSE(character.conditions.has(CONDITION_DEAD));
        EXPECT_FALSE(character.pCharacterBuffs[static_cast<CharacterBuff>(0)].Active());
    }
}

UNIT_TEST(MM6PartyDeath, GuardianAngelKeepsHalfTheGoldAndRaisesByMastery) {
    // Any Guardian Angel halves the gold loss, only Expert and Master raise the party above 1 HP.
    for (Mastery mastery : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
        auto party = makeDeadParty(mastery);
        mm6::applyPartyDeath(party.get(), [&](const Character &character) { return maxHealthByIndex(*party, character); });

        EXPECT_EQ(party->uNumGold, 50);
        EXPECT_FALSE(party->pPartyBuffs[static_cast<PartyBuff>(8)].Active());
        for (int i = 0; i < 4; i++) {
            const int maxHealth = 21 + 20 * i;
            const int expected = mastery == MASTERY_MASTER ? maxHealth : mastery == MASTERY_EXPERT ? maxHealth / 2 : 1;
            EXPECT_EQ(party->pCharacters[i].health, expected);
        }
    }
}
