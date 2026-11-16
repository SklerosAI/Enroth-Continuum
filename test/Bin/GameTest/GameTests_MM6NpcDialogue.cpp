#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

#include "Library/Image/Png.h"

#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

static const Vec3f kMarianneSpot(-10333, -8532, 300); // In front of the general store, Marianne the Teacher stands close by.

static void enterNewSorpigal(EngineController &game) {
    // Default party through the creation screen, then New Sorpigal. The test-only new-game callback seeds the map
    // cache from the loaded archives and writes no save.
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
    game.runGameRoutine([] {
        for (Character &character : pParty->pCharacters) {
            for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
                const Skill skill = character.GetSkillIdxByOrder(order);
                if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                    character.pActiveSkills[skill] = CombinedSkillValue::novice();
            }
        }
        for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
            for (Character &character : pParty->pCharacters)
                for (Attribute stat : allStatAttributes())
                    character.IncreaseAttribute(stat);
    });
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map. Nothing stands in for the new game's own save.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void talkToMarianne(EngineController &game) {
    // Teleport next to the street NPC and talk to her with Space.
    game.teleportTo(engine->_currentLoadedMapId, kMarianneSpot, 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void captureFrame(EngineController &game, std::string_view prefix, std::string_view name) {
    RgbaImage frame;
    game.runGameRoutine([&frame] {
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot();
    });
    FileOutputStream output{Path(fmt::format("{}-{}.png", prefix, name))};
    output.write(png::encode(frame));
    output.close();
}

static std::vector<Recti> dialogueButtonRects() {
    // Hit boxes of the conversation window's exit and topic buttons, empty while no conversation is open.
    std::vector<Recti> result;
    if (pDialogueWindow)
        for (GUIButton *button : pDialogueWindow->vButtons)
            if (button->id.starts_with("NpcDialogue_"))
                result.push_back(button->rect);
    return result;
}

GAME_TEST(MM6NpcDialogue, StreetNpcPanel) {
    // Talking to a street NPC drew MM7's conversation frame over MM6's right panel, with placeholders for the MM7 art
    // that MM6 doesn't have, and its topics and exit sat where MM7 puts them.
    auto screenTape = tapes.screen();
    auto texturesTape = tapes.hudTextures();
    auto textTape = tapes.allGUIWindowsText();
    auto rectsTape = tapes.custom([] { return dialogueButtonRects(); });
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    game.pressAndReleaseButton(BUTTON_LEFT, 555, 200); // More Information.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 560, 325); // The exit picture.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME));
    EXPECT_EQ(rectsTape, tape(std::vector<Recti>(),
                              std::vector<Recti>{{526, 313, 75, 33}, {480, 185, 140, 32}, {480, 248, 140, 16}},
                              std::vector<Recti>()));
    auto textures = texturesTape.flatten();
    EXPECT_CONTAINS(textures, "evpan019");
    EXPECT_CONTAINS(textures, "npc260");
    EXPECT_CONTAINS(textures, "buttesc1");
    EXPECT_MISSES(textures, "evt02");
    EXPECT_MISSES(textures, "evtnpc");
    EXPECT_MISSES(textures, "ib-bcu-a");
    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "Marianne the Teacher");
    EXPECT_CONTAINS(text, "More Information");
    EXPECT_CONTAINS(text, "Hire");
    EXPECT_CONTAINS(text, "Ten percent bonus on all experience learned. \nMarianne takes 3 percent of all gold you find.");
}

GAME_TEST(MM6NpcDialogue, StreetNpcHire) {
    // Clicking Hire where MM6 draws it asks the party for the street NPC's fee.
    auto hirelingsTape = tapes.totalHirelings();
    auto goldTape = tapes.gold();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    game.pressAndReleaseButton(BUTTON_LEFT, 555, 255); // Hire.
    game.tick();

    EXPECT_EQ(hirelingsTape, tape(0));
    EXPECT_EQ(goldTape, tape(200));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE));
    EXPECT_EQ(statusTape, tape("Rest", "", "You don't have enough gold"));
}

GAME_TEST(MM6NpcDialogue, StreetNpcEscape) {
    // One Escape closes a street NPC's conversation.
    auto screenTape = tapes.screen();
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME));
}

GAME_TEST(MM6NpcDialogue, NewGameAfterHire) {
    // Hiring a street NPC and then starting a new game kept the hireling's name behind a NUL, and the new game
    // couldn't be saved.
    startNewGameThroughInput(game);
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    auto hirelingsTape = tapes.totalHirelings();
    auto statusTape = tapes.statusBar();
    test.startTaping();

    talkToMarianne(game);
    game.pressAndReleaseButton(BUTTON_LEFT, 555, 255); // Hire.
    game.tick();
    game.goToMainMenu();
    startNewGameThroughInput(game);

    EXPECT_EQ(hirelingsTape, tape(0, 1, 0));
    EXPECT_MISSES(statusTape, "This party can't be saved yet");
}

GAME_TEST(MM6OffscreenFrame, DISABLED_NpcDialogueCapture) {
    // Hidden capture of a street NPC's conversation, before and after its first topic, and of the HUD once she's hired.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePrefix = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePrefix, nullptr);
    ASSERT_TRUE(std::string_view(capturePrefix).starts_with("/private/tmp/"));

    enterNewSorpigal(game);
    talkToMarianne(game);
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    captureFrame(game, capturePrefix, "talk");
    game.pressAndReleaseButton(BUTTON_LEFT, 555, 200); // More Information.
    game.tick();
    captureFrame(game, capturePrefix, "topic");
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    game.pressAndReleaseButton(BUTTON_LEFT, 555, 255); // Hire.
    game.tick();
    captureFrame(game, capturePrefix, "hired");
}
