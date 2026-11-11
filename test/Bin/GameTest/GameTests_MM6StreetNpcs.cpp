#include <algorithm>
#include <memory>
#include <stdexcept>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"

static void completeEditableMM6Party() {
    // Four skills for every character and no unspent attribute points, so that the creation screen accepts the party.
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

GAME_TEST(MM6StreetNpcs, Outc2LoadsWithItsStreetNpcs) {
    // Entering outc2 threw a generated NPC pool overflow, because MM7's peasant ID ranges picked well over a hundred
    // of its monsters for a street NPC.
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto streetPeasantsTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return mm6::nativeStreetNpcEligible(actor.monsterInfo.id); });
    });
    auto streetLinksTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.npcId >= 5000 && actor.npcId < 5000 + pNPCStats->uNewlNPCBufPos; });
    });
    auto otherLinksTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.npcId != 0 && actor.npcId < 5000; });
    });
    auto poolTape = tapes.custom([] { return pNPCStats->uNewlNPCBufPos; });
    enterFirstMapFromNewGame(game);

    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.teleportTo(pMapTable->GetMapInfo("outc2.odm"), Vec3f(11407, 18074, 1), 0); // Where the d08 exit puts the party.
    game.runGameRoutine([&] { chestOptIn.reset(); });
    test.startTaping();
    game.tick();

    EXPECT_EQ(mapTape, tape(pMapTable->GetMapInfo("outc2.odm")));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME));
    EXPECT_EQ(streetPeasantsTape, tape(58));
    EXPECT_EQ(streetLinksTape, tape(58));
    EXPECT_EQ(poolTape, tape(58));
    EXPECT_EQ(otherLinksTape, tape(0));
}
