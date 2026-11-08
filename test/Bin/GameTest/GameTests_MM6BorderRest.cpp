#include <memory>
#include <stdexcept>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6OutdoorBorderConfirmation.h"
#include "Engine/MM6/MM6PartyRest.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

static void completeEditableMM6Party() {
    for (Character &character : pParty->pCharacters) {
        for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
            const Skill skill = character.GetSkillIdxByOrder(order);
            if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                character.pActiveSkills[skill] = CombinedSkillValue::novice();
        }
        if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
            throw std::runtime_error("MM6 creation party lacks four skills");
    }
    for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
        for (Character &character : pParty->pCharacters)
            for (Attribute stat : allStatAttributes())
                character.IncreaseAttribute(stat);
    if (CharacterCreation_GetUnspentAttributePointCount() != 0)
        throw std::runtime_error("MM6 creation party could not spend attribute points");
}

static void enterFirstMapFromNewGame(EngineController &game) {
    // Default party through the creation screen, then the first map in the real game loop. The test-only new-game
    // callback seeds the map cache from the loaded archives and writes no save.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
        pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
        mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

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

GAME_TEST(MM6BorderRest, DISABLED_DayRolloverSkipsTheTripDays) {
    // The days of a confirmed border trip bump the no-rest counter once, the day rollover doesn't count them again.
    // Run alone in a fresh MM6 headless process.
    auto noRestTape = tapes.custom([] { return static_cast<int>(pParty->days_played_without_rest); });
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime(); });
    enterFirstMapFromNewGame(game);

    pParty->uNumFoodRations = 0;
    pParty->pos = Vec3f(-22529, pParty->pos.y, pParty->pos.z);
    const mm6::BorderTravelCandidate offered =
        *mm6::assessOutdoorBorderTravel(GameTitle::MM6, "oute3.odm", pParty->pos.x, pParty->pos.y, {}).candidate;
    mm6::BorderConfirmation confirmed =
        mm6::stageBorderConfirmation(GameTitle::MM6, "oute3.odm", offered, *pParty, {}, mm6::restAndHealDetachedParty);
    ASSERT_EQ(confirmed.refusal, mm6::BorderConfirmationRefusal::None);
    ASSERT_EQ(confirmed.effects.travelDays, 5);
    const Time arrived = confirmed.party->GetPlayingTime();
    game.runGameRoutine([&] { *pParty = std::move(*confirmed.party); });
    test.startTaping();
    game.tick();

    EXPECT_EQ(noRestTape, tape(1));
    EXPECT_GE(timeTape.front(), arrived);
}
