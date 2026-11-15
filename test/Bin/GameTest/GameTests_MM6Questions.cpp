#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/MM6MapByteState.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "Library/Image/Png.h"

#include "Utility/Streams/FileOutputStream.h"

#include "MM6CampaignChain.h"

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
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

static void teleportToMap(EngineController &game, const char *map, Vec3f position, int viewYaw) {
    // A test teleport loads the map the way a trip there would, with its chests.
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.teleportTo(pMapTable->GetMapInfo(map), position, viewYaw);
    game.runGameRoutine([&] { chestOptIn.reset(); });
}

static bool mapByteSet(int rawId) {
    return MM6MapByteState(pIndoor->mm6DeltaBytes).compare(rawId, 1);
}

static void enterCastleAlamos(EngineController &game) {
    startNewGameThroughInput(game);
    teleportToMap(game, "cd1.blv", Vec3f(-2921, 13139, 225), 0); // Where oute1's way in puts the party.
}

GAME_TEST(MM6Questions, Cd1PasswordPlate) {
    // Castle Alamos asks for the password on a plate before its inner rooms. MM6 compares the typed text as it is, so
    // a trailing space makes the right password wrong.
    for (std::string_view answer : {"jbard", "jbard "}) {
        const bool right = answer == "jbard";
        test.prepareForNextTest();
        enterCastleAlamos(game);
        walkOntoMM6CampaignChainIndoorEventPlate(game, 69);
        auto screenTape = tapes.screen();
        auto statusTape = tapes.statusBar();
        auto positionTape = tapes.custom([] { return pParty->pos; });
        auto passedTape = tapes.custom([] { return mapByteSet(111); });
        test.startTaping();

        answerMM6CampaignChainQuestion(game, answer);

        EXPECT_EQ(screenTape, tape(SCREEN_BRANCHLESS_NPC_DIALOG, SCREEN_GAME));
        if (right) {
            EXPECT_EQ(statusTape.back(), "Who told you!  Alright, you may pass!");
            EXPECT_EQ(passedTape, tape(false, true));
            EXPECT_EQ(positionTape.size(), 1);
        } else {
            EXPECT_EQ(statusTape.back(), "Wrong!");
            EXPECT_EQ(passedTape, tape(false));
            EXPECT_EQ(positionTape.back(), Vec3f(-3136, 2240, 224));
        }
    }
}

GAME_TEST(MM6Questions, Cd1PasswordEscape) {
    // Escape at the Castle Alamos password prompt stopped the plate's event. MM6 takes it for an empty answer, which is
    // a wrong password.
    enterCastleAlamos(game);
    walkOntoMM6CampaignChainIndoorEventPlate(game, 69);
    ASSERT_EQ(current_screen_type, SCREEN_BRANCHLESS_NPC_DIALOG);
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto passedTape = tapes.custom([] { return mapByteSet(111); });
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME));
    EXPECT_EQ(statusTape.back(), "Wrong!");
    EXPECT_EQ(passedTape, tape(false));
    EXPECT_EQ(positionTape.back(), Vec3f(-3136, 2240, 224));
}

GAME_TEST(MM6Questions, Cd1PasswordPromptOnStatusLine) {
    // The password prompt drew the question in the message box and the typed text alone on the status line. MM6 keeps
    // the plate's message in the box and draws the question with the typed text after it on the status line.
    enterCastleAlamos(game);
    walkOntoMM6CampaignChainIndoorEventPlate(game, 69);
    auto textTape = tapes.drawnTexts();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_J);
    game.pressAndReleaseKey(PlatformKey::KEY_B);
    game.tick();

    auto texts = textTape.flatten();
    EXPECT_CONTAINS(texts.map([](const DrawnText &text) { return text.text; }), "Restricted area - Keep out.");
    EXPECT_CONTAINS(texts, (DrawnText{"What's the password? jb", colorTable.White, {13, 355}}));
}

GAME_TEST(MM6Questions, PyramidFirstMateCode) {
    // A Cleansing Pool in the Tomb of VARN asks for the first mate's code when the party holds his code scroll.
    for (std::string_view answer : {"kcops", "kirk"}) {
        const bool right = answer == "kcops";
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        teleportToMap(game, "pyramid.blv", Vec3f(-9734, -19201, 772), 90); // Where outb3's way in puts the party.
        game.runGameRoutine([] {
            pParty->setHoldingItem(Item(static_cast<ItemId>(538))); // First Mate's Code.
            pParty->_questBits.set(static_cast<QuestBit>(229)); // The map sets it when the party opens chest 2.
        });
        auto statusTape = tapes.statusBar();
        auto heldTape = tapes.custom([] { return std::to_underlying(pParty->pPickedItem.itemId); });
        auto bitTape = tapes.questBit(static_cast<QuestBit>(229));
        auto codeTape = tapes.custom([] { return mapByteSet(115); });
        auto hpTape = charTapes.hps();
        walkOntoMM6CampaignChainIndoorEventPlate(game, 21);
        test.startTaping();

        answerMM6CampaignChainQuestion(game, answer);

        if (right) {
            EXPECT_EQ(heldTape, tape(538, 0));
            EXPECT_EQ(bitTape, tape(true, false));
            EXPECT_EQ(codeTape, tape(false, true));
            EXPECT_EQ(hpTape.size(), 1);
        } else {
            EXPECT_EQ(statusTape.back(), "Incorrect.");
            EXPECT_EQ(heldTape, tape(538));
            EXPECT_EQ(bitTape, tape(true));
            EXPECT_EQ(codeTape, tape(false));
            EXPECT_EQ(hpTape.delta(), tape(-5, 0, 0, 0));
        }
    }
}

GAME_TEST(MM6OffscreenFrame, DISABLED_QuestionCapture) {
    // Hidden capture of the Castle Alamos password prompt with two letters typed.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePath = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    ASSERT_TRUE(std::string_view(capturePath).starts_with("/private/tmp/"));
    enterCastleAlamos(game);
    walkOntoMM6CampaignChainIndoorEventPlate(game, 69);
    game.pressAndReleaseKey(PlatformKey::KEY_J);
    game.pressAndReleaseKey(PlatformKey::KEY_B);
    game.tick();

    RgbaImage frame;
    game.runGameRoutine([&frame] {
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot();
    });
    FileOutputStream output{Path(capturePath)};
    output.write(png::encode(frame));
    output.close();
}
