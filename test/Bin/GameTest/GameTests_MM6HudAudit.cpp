#include <cctype>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Viewport.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/NPC.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/HudNpcArrowResources.h"

#include "MM6MapEntranceForTest.h"

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

static void enterMapAtShippedEntrance(EngineController &game, std::string_view fileName) {
    // Loads the map at the first shipped MoveToMap that names it, or at its party start without one, the way the map
    // sweep does.
    const MapId map = pMapTable->GetMapInfo(fileName);
    const std::optional<MM6ShippedEntrance> entrance = findShippedMM6Entrance(fileName);
    bool loaded = false;
    game.runGameRoutine([&] {
        for (Chest &chest : vChests)
            chest.position.reset(); // The first map's chest positions would make the target's chest load refuse.
        engine->_pendingTransition = entrance ? MapDestination(map, entrance->placement) : MapDestination(map, MAP_START_POINT_PARTY);
        mm6::ScopedChestMapLoadOptIn chestOptIn;
        PrepareWorld(0);
        loaded = true;
    });
    while (!loaded)
        game.tick(); // The loading screen draws frames.
    ASSERT_EQ(engine->_currentLoadedMapId, map);
}

static DrawnQuad withoutAnimationFrame(DrawnQuad quad) {
    // Buff sprite frames are numbered, i2_fire1 and on. Which frame is up depends on how long the loading screen ran.
    if (quad.texture.size() > 3 && quad.texture[2] == '_')
        while (std::isdigit(static_cast<unsigned char>(quad.texture.back())))
            quad.texture.pop_back();
    return quad;
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

GAME_TEST(MM6Hud, MinimapFillsTopPanelHole) {
    // The minimap was drawn in MM7's 137x117 rect, which left the edges of the hole in the top panel unpainted.
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.tick();

    auto topPanel = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position.x >= 468 && quad.position.y < 141; });
    EXPECT_EQ(topPanel, tape(DrawnQuad{"", {480, 25}}, DrawnQuad{"mapdir2", {553, 79}}, DrawnQuad{"tap2", {468, 0}})); // The minimap has no name.
}

GAME_TEST(MM6Hud, IndoorMinimapBackground) {
    // Indoors, the minimap was a navy blue fill instead of MM6's MAPBACK art.
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    enterMapAtShippedEntrance(game, "d02.blv");
    test.startTaping();

    game.tick();

    auto topPanel = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position.x >= 468 && quad.position.y < 141; });
    EXPECT_EQ(topPanel, tape(DrawnQuad{"mapback", {482, 25}}, DrawnQuad{"mapdir2", {553, 79}}, DrawnQuad{"tap2", {468, 0}}));
}

GAME_TEST(MM6Hud, ZoomCircles) {
    // Clicking the minus and plus marks on the top panel didn't zoom the minimap, the zoom buttons sat below it.
    auto zoomTape = tapes.custom([] { return viewparams->uMinimapZoom; });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseButton(BUTTON_LEFT, 595, 42); // Plus.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 502, 42); // Minus.
    game.tick();

    EXPECT_EQ(zoomTape, tape(1024, 512));
}

GAME_TEST(MM6Hud, PartyBuffSprites) {
    // Party buffs were drawn as MM7's isn icons, which MM6 doesn't have, instead of MM6's sprites on the shelf.
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    game.runGameRoutine([] {
        for (PartyBuff buff : {PARTY_BUFF_RESIST_FIRE, PARTY_BUFF_RESIST_WATER, PARTY_BUFF_RESIST_AIR, PARTY_BUFF_PROTECTION_FROM_MAGIC,
                               PARTY_BUFF_RESIST_BODY, PARTY_BUFF_FEATHER_FALL})
            pParty->pPartyBuffs[buff].Apply(pParty->GetPlayingTime() + Duration::fromHours(1), MASTERY_EXPERT, 1, 0, 0);
    });
    test.startTaping();

    game.tick();

    auto shelf = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position.x >= 468 && quad.position.y > 141 && quad.position.y < 260; });
    EXPECT_EQ(shelf.map(withoutAnimationFrame), tape(DrawnQuad{"i2_fire", {513, 231}}, DrawnQuad{"i4_cold", {555, 231}}, DrawnQuad{"i3_ltng", {530, 231}},
                                                     DrawnQuad{"i6_magc", {597, 231}}, DrawnQuad{"i5_pois", {579, 231}}, DrawnQuad{"i1_ffal", {490, 231}}));
}

GAME_TEST(MM6Hud, TorchLightAndWizardEye) {
    // An active torch light or wizard eye crashed MM6 on MM7's torchA and wizeyeA icon animations.
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    game.runGameRoutine([] {
        for (PartyBuff buff : {PARTY_BUFF_TORCHLIGHT, PARTY_BUFF_WIZARD_EYE})
            pParty->pPartyBuffs[buff].Apply(pParty->GetPlayingTime() + Duration::fromHours(1), MASTERY_EXPERT, 1, 0, 0);
    });
    test.startTaping();

    game.tick();

    auto topPanel = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position.x >= 468 && quad.position.y < 141; });
    EXPECT_EQ(topPanel, tape(DrawnQuad{"", {480, 25}}, DrawnQuad{"mapdir2", {553, 79}}, DrawnQuad{"tap2", {468, 0}}, DrawnQuad{"wizeye", {547, 20}})); // No torch light.
}

GAME_TEST(MM6Hud, HitPointBarSegments) {
    // Hit point bars were one green bar filled linearly, MM6 switches to the half and quarter images as hit points drop.
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    game.runGameRoutine([] {
        pParty->pCharacters[0].health = pParty->pCharacters[0].GetMaxHealth() * 2 / 5;
        pParty->pCharacters[1].health = pParty->pCharacters[1].GetMaxHealth() / 5;
    });
    test.startTaping();

    game.tick();

    auto bars = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.texture.starts_with("hits"); });
    EXPECT_EQ(bars, tape(DrawnQuad{"hitshalf", {93, 421}}, DrawnQuad{"hitsqtr", {206, 442}},
                         DrawnQuad{"hitsfull", {319, 383}}, DrawnQuad{"hitsfull", {431, 383}}));
}

GAME_TEST(MM6Hud, HirelingPortraitsAndArrows) {
    // Hireling portraits sat two pixels off their frames, and the NPC arrows only showed while pressed.
    auto quadsTape = tapes.hudQuads();
    auto scrollTape = tapes.custom([] { return pParty->hirelingScrollPosition; });
    enterFirstMapFromNewGame(game);
    game.runGameRoutine([] {
        pParty->pHirelings[0] = pNPCStats->pNPCData[1];
        pParty->pHirelings[1] = pNPCStats->pNPCData[2];
        pNPCStats->pNPCData[3].flags |= NPC_HIRED; // A follower, so there are three to scroll through.
        pParty->CountHirelings();
    });
    test.startTaping();

    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 632, 186); // The right arrow.
    game.tick();

    auto panel = quadsTape.flatten().filter([](const DrawnQuad &quad) {
        return quad.position.x >= 468 && quad.position.y >= 149 && quad.position.y < 230 && !quad.texture.starts_with("micon"); // Not the cursor.
    });
    EXPECT_EQ(scrollTape, tape(0, 1));
    EXPECT_EQ(panel, tape(DrawnQuad{"npc081", {491, 149}}, DrawnQuad{"npc126", {561, 149}}, DrawnQuad{"npc_rup", {626, 174}},
                          DrawnQuad{"npc126", {491, 149}}, DrawnQuad{"npc187", {561, 149}}, DrawnQuad{"npc_lup", {474, 174}}, // The click redraws them.
                          DrawnQuad{"npc126", {491, 149}}, DrawnQuad{"npc187", {561, 149}}, DrawnQuad{"npc_lup", {474, 174}},
                          DrawnQuad{"npc_rdn", {626, 174}}));
}

GAME_TEST(MM6Hud, HoverHitBoxes) {
    // The date, hit point and portrait hints used MM7's boxes, so the top of the panel, the gap between a character's
    // bars and the left edge of a portrait showed nothing.
    auto statusTape = tapes.statusBar();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.moveMouse(600, 5); // Top of the panel, right of the compass.
    game.tick();
    game.moveMouse(99, 420); // Between the first character's bars.
    game.tick();
    game.moveMouse(20, 422); // Left edge of the first portrait.
    game.tick();

    EXPECT_EQ(statusTape, tape("9:00am Monday 1 January 1165", "38 / 38 Hit Points    0 / 0 Spell Points", "Zoltan the Knight: Good"));
}

GAME_TEST(MM6Hud, BookAndActionHitBoxes) {
    // The book and action boxes were a few pixels off, so the right edge of the maps book opened the calendar and the
    // right edge of the quick reference oval did nothing.
    auto screenTape = tapes.screen();
    auto windowTape = tapes.custom([] { return pGUIWindow_CurrentMenu ? pGUIWindow_CurrentMenu->eWindowType : WINDOW_NULL; });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseButton(BUTTON_LEFT, 590, 300); // Right edge of the calendar book.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 589, 430); // Right edge of the quick reference oval.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_BOOKS, SCREEN_GAME, SCREEN_QUICK_REFERENCE));
    EXPECT_EQ(windowTape, tape(WINDOW_CALENDAR_BOOK, WINDOW_NULL, WINDOW_QUICK_REFERENCE));
}
