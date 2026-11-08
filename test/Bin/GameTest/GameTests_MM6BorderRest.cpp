#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MM6/MM6OutdoorBorderConfirmation.h"
#include "Engine/MM6/MM6PartyRest.h"
#include "Engine/Party.h"

#include "GUI/GUIWindow.h"

GAME_TEST(MM6BorderRest, ConfirmationRestsOnlyTheCopy) {
    // Confirming a border crossing rests the party copy as if it were the live party, and leaves the live party alone.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);

    // The characters turn a hundred, the age when endurance starts to drop, during the trip.
    pParty->GetPlayingTime() = Time::fromYears(10) - Duration::fromDays(2);
    for (Character &character : pParty->pCharacters) {
        character.uBirthYear = gameStartingYear - 90;
        character._stats[ATTRIBUTE_ENDURANCE] = 100;
        character.health = 1;
        character.mana = 0;
    }
    pParty->pCharacters[0].conditions.set(CONDITION_WEAK, pParty->GetPlayingTime());
    pParty->pPartyBuffs[PARTY_BUFF_DAY_OF_GODS].Apply(pParty->GetPlayingTime() + Duration::fromDays(30), MASTERY_MASTER, 20, 0, 0);
    pParty->days_played_without_rest = 3;
    pParty->uNumFoodRations = 10;
    pParty->pos = Vec3f(22529, 77, 0);
    const Time departure = pParty->GetPlayingTime();

    Party restedAtDeparture = *pParty;
    mm6::restAndHealDetachedParty(restedAtDeparture);
    const mm6::BorderTravelCandidate offered =
        *mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", pParty->pos.x, pParty->pos.y, {}).candidate;
    const mm6::BorderConfirmation confirmed =
        mm6::stageBorderConfirmation(GameTitle::MM6, "outc2.odm", offered, *pParty, {}, mm6::restAndHealDetachedParty);
    ASSERT_EQ(confirmed.refusal, mm6::BorderConfirmationRefusal::None);
    const Party &copy = *confirmed.party;

    EXPECT_EQ(pParty->GetPlayingTime(), departure);
    EXPECT_EQ(pParty->pPartyBuffs[PARTY_BUFF_DAY_OF_GODS].power, 20);
    EXPECT_EQ(pParty->days_played_without_rest, 3);
    EXPECT_TRUE(pParty->pCharacters[0].conditions.has(CONDITION_WEAK));
    for (const Character &character : pParty->pCharacters) {
        EXPECT_EQ(character.health, 1);
        EXPECT_EQ(character.mana, 0);
    }

    EXPECT_FALSE(copy.pPartyBuffs[PARTY_BUFF_DAY_OF_GODS].Active());
    EXPECT_EQ(copy.days_played_without_rest, 0);
    EXPECT_FALSE(copy.pCharacters[0].conditions.has(CONDITION_WEAK));
    EXPECT_NE(copy.pCharacters[0].health, restedAtDeparture.pCharacters[0].health);

    pParty->GetPlayingTime() = copy.playing_time;
    pParty->restAndHeal();
    for (size_t i = 0; i < copy.pCharacters.size(); ++i) {
        EXPECT_GT(copy.pCharacters[i].health, 1);
        EXPECT_EQ(copy.pCharacters[i].health, pParty->pCharacters[i].health);
        EXPECT_EQ(copy.pCharacters[i].mana, pParty->pCharacters[i].mana);
    }
}
