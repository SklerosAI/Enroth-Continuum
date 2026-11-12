#include <memory>
#include <stdexcept>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/HudNpcArrowResources.h"

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

GAME_TEST(MM6HudAudit, NativeNpcArrows) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudNpcArrowResources arrows = hudNpcArrowResources(GameTitle::MM6,
        "IB-NPCLD-A", "IB-NPCRD-A");
    EXPECT_EQ(arrows.left, "npc_lup");
    EXPECT_EQ(arrows.right, "npc_rup");
    EXPECT_EQ(engine->resources()->iconSize(arrows.left), Sizei(14, 25));
    EXPECT_EQ(engine->resources()->iconSize(arrows.right), Sizei(14, 25));
}

TEST(MM6HudAuditResources, MM7KeepsAlignmentVariants) {
    const auto neutral = hudNpcArrowResources(GameTitle::MM7, "IB-NPCLD-A", "IB-NPCRD-A");
    const auto good = hudNpcArrowResources(GameTitle::MM7, "IB-NPCLD-B", "IB-NPCRD-B");
    const auto evil = hudNpcArrowResources(GameTitle::MM7, "IB-NPCLD-C", "IB-NPCRD-C");
    EXPECT_EQ(neutral.left, "IB-NPCLD-A");
    EXPECT_EQ(neutral.right, "IB-NPCRD-A");
    EXPECT_EQ(good.left, "IB-NPCLD-B");
    EXPECT_EQ(good.right, "IB-NPCRD-B");
    EXPECT_EQ(evil.left, "IB-NPCLD-C");
    EXPECT_EQ(evil.right, "IB-NPCRD-C");
}

GAME_TEST(MM6Hud, FoodAndGoldHover) {
    // Hovering the apple or the coin stack showed no hint, the hover boxes sat where MM7 draws its numbers.
    auto statusTape = tapes.statusBar();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.moveMouse(498, 373); // The apple.
    game.tick();
    game.moveMouse(616, 369); // The coin stack.
    game.tick();

    EXPECT_EQ(statusTape, tape("You have 7 food", "You have 200 gold, 0 in the Bank"));
}

GAME_TEST(MM6Hud, FoodAndGoldNumbers) {
    // Gold was drawn five pixels left of its slot, and as 1.235M once it passed a million.
    auto textTape = tapes.allGUIWindowsText();
    enterFirstMapFromNewGame(game);
    game.runGameRoutine([] { pParty->SetGold(1234567, /*silent=*/true); });
    test.startTaping();

    game.tick();

    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "\r0877");
    EXPECT_CONTAINS(text, "\r0231234567");
}

GAME_TEST(MM6Hud, CompassUnderTopPanelSlot) {
    // The compass was drawn at y 136 under the opaque top panel art, so it never showed.
    auto quadsTape = tapes.hudQuads();
    auto yawTape = tapes.custom([] { return pParty->_viewYaw; });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.tick();
    game.pressKey(PlatformKey::KEY_LEFT);
    game.tick();
    game.releaseKey(PlatformKey::KEY_LEFT);

    auto compassAndPanel = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.texture == "compass" || quad.texture == "tap2"; });
    EXPECT_EQ(yawTape, tape(512, 616));
    EXPECT_EQ(compassAndPanel, tape(DrawnQuad{"compass", {348, 10}}, DrawnQuad{"tap2", {468, 0}},
                                    DrawnQuad{"compass", {361, 10}}, DrawnQuad{"tap2", {468, 0}}));
}
