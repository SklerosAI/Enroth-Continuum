#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <variant>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PartyDeath.h"
#include "Engine/Party.h"

namespace {

std::unique_ptr<Party> makeParty() {
    auto party = std::make_unique<Party>();
    party->playing_time = Time::fromDays(10);
    party->uNumDeaths = 2;
    party->uNumGold = 101;
    party->pos = Vec3f(3, 4, 5);
    party->velocity = Vec3f(6, 7, 8);
    party->_viewYaw = 100;
    party->_viewPitch = 20;
    party->pPartyBuffs[PARTY_BUFF_RESIST_AIR].power = 3;
    for (Character &character : party->pCharacters) {
        character.health = 5;
        character.mana = 9;
        character.conditions.set(CONDITION_DEAD, Time::fromTicks(1));
        character.pCharacterBuffs[CHARACTER_BUFF_BLESS].power = 2;
    }
    return party;
}

const mm6::PartyDeathPlan &plan(const mm6::PartyDeathResult &result) {
    return std::get<mm6::PartyDeathPlan>(result);
}

} // namespace

UNIT_TEST(MM6PartyDeath, OrdinaryDeathStagesSourceBackedRespawn) {
    auto source = makeParty();
    const auto result = mm6::planPartyDeath(*source, GameTitle::MM6,
                                           mm6::GuardianAngelState::Absent, {});
    ASSERT_TRUE(std::holds_alternative<mm6::PartyDeathPlan>(result));
    const auto &staged = *plan(result).party;
    EXPECT_EQ(plan(result).mapFile, "oute3.odm");
    EXPECT_EQ(staged.pos, Vec3f(-9728, -11319, 160));
    EXPECT_EQ(staged.velocity, Vec3f());
    EXPECT_EQ(staged._viewYaw, 512);
    EXPECT_EQ(staged._viewPitch, 0);
    EXPECT_EQ(staged.uNumDeaths, 3);
    EXPECT_EQ(staged.playing_time, Time::fromDays(17));
    EXPECT_EQ(staged.uNumGold, 0);
    EXPECT_EQ(staged.pPartyBuffs[PARTY_BUFF_RESIST_AIR].power, 0);
    for (const Character &character : staged.pCharacters) {
        EXPECT_EQ(character.health, 1);
        EXPECT_EQ(character.mana, 9); // The original death block does not write mana.
        EXPECT_FALSE(character.conditions.has(CONDITION_DEAD));
        EXPECT_EQ(character.pCharacterBuffs[CHARACTER_BUFF_BLESS].power, 0);
    }
    EXPECT_EQ(source->uNumDeaths, 2);
    EXPECT_EQ(source->uNumGold, 101);
    EXPECT_EQ(source->pCharacters[0].health, 5);
    EXPECT_TRUE(source->pCharacters[0].conditions.has(CONDITION_DEAD));
}

UNIT_TEST(MM6PartyDeath, GuardianAngelExpertAndMasterUseExplicitHealth) {
    auto source = makeParty();
    constexpr std::array<int, 4> maximumHealth{21, 40, 79, 100};
    auto expert = mm6::planPartyDeath(*source, GameTitle::MM6,
                                      mm6::GuardianAngelState::Expert, maximumHealth);
    ASSERT_TRUE(std::holds_alternative<mm6::PartyDeathPlan>(expert));
    EXPECT_EQ(plan(expert).party->uNumGold, 50);
    EXPECT_EQ(plan(expert).party->pCharacters[0].health, 10);
    EXPECT_EQ(plan(expert).party->pCharacters[3].health, 50);

    auto master = mm6::planPartyDeath(*source, GameTitle::MM6,
                                      mm6::GuardianAngelState::Master, maximumHealth);
    ASSERT_TRUE(std::holds_alternative<mm6::PartyDeathPlan>(master));
    EXPECT_EQ(plan(master).party->uNumGold, 50);
    EXPECT_EQ(plan(master).party->pCharacters[0].health, 21);
    EXPECT_EQ(plan(master).party->pCharacters[3].health, 100);
    EXPECT_EQ(source->uNumGold, 101);
}

UNIT_TEST(MM6PartyDeath, RefusesUnknownAndUnmappedStateBeforeMutation) {
    auto source = makeParty();
    constexpr std::array<int, 4> maximumHealth{21, 40, 79, 100};
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Unknown, maximumHealth)),
        mm6::PartyDeathRefusal::UnknownGuardianAngel);
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM7, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::ForeignTitle);

    source->pPartyBuffs[static_cast<PartyBuff>(16)].power = 1;
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::UnmappedBuff);
    source->pPartyBuffs[static_cast<PartyBuff>(16)] = SpellBuff();
    source->pCharacters[0].pCharacterBuffs[static_cast<CharacterBuff>(16)].power = 1;
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::UnmappedBuff);
    source->pCharacters[0].pCharacterBuffs[static_cast<CharacterBuff>(16)] = SpellBuff();
    source->pCharacters[0].conditions.set(CONDITION_ZOMBIE, Time::fromTicks(1));
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::UnmappedCondition);
    EXPECT_EQ(source->uNumDeaths, 2);
    EXPECT_EQ(source->pCharacters[0].health, 5);
}

UNIT_TEST(MM6PartyDeath, RefusesOverflowAndInvalidHealth) {
    auto source = makeParty();
    source->uNumGold = -1;
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::InvalidGold);
    source->uNumGold = 101;
    source->uNumDeaths = std::numeric_limits<int>::max();
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::DeathCountOverflow);
    source->uNumDeaths = 2;
    source->playing_time = Time::fromTicks(std::numeric_limits<std::int64_t>::max());
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Absent, {})),
        mm6::PartyDeathRefusal::PlayingTimeOverflow);
    source->playing_time = Time::fromDays(10);
    EXPECT_EQ(std::get<mm6::PartyDeathRefusal>(mm6::planPartyDeath(
        *source, GameTitle::MM6, mm6::GuardianAngelState::Expert, {21, 0, 79, 100})),
        mm6::PartyDeathRefusal::InvalidMaximumHealth);
}
