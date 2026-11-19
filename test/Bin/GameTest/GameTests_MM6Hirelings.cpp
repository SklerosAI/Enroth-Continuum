#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Resources/ResourceManager.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

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

static void talkToStreetNpc(EngineController &game, int actorId) {
    // Stand next to the street NPC, facing east towards her, and click on her.
    game.teleportTo(engine->_currentLoadedMapId, pActors[actorId].pos - Vec3f(150, 0, 0), 0);
    game.pointMouseAtActor(actorId);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static void clickNpcOption(EngineController &game, std::string_view label) {
    // Left click in the middle of the conversation topic with this label.
    ASSERT_NE(pDialogueWindow, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : pDialogueWindow->vButtons)
        if (button->id.starts_with("NpcDialogue_Option") && button->label == label)
            center = button->rect.center();
    ASSERT_TRUE(center) << "No conversation topic " << label;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

GAME_TEST(MM6Hirelings, StreetNpcPortraitExists) {
    // Talking to a street NPC drew a missing image for her portrait, she had one of MM7's elf portraits.
    startNewGameThroughInput(game);
    auto quadsTape = tapes.hudQuads();
    test.startTaping();

    talkToStreetNpc(game, 4); // Eleanor the Acolyte.

    auto portraits = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position == Pointi(525, 34); });
    EXPECT_EQ(portraits, tape(DrawnQuad{"npc301", {525, 34}}));
    for (const DrawnQuad &quad : portraits)
        EXPECT_GT(engine->resources()->iconSize(quad.texture).w, 0) << quad.texture;
}

GAME_TEST(MM6Hirelings, HireTwoAndRefuseAThird) {
    // The party hires two street NPCs for their fees and sees them in the right panel. A third one refuses to join.
    startNewGameThroughInput(game);
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    auto hirelingsTape = tapes.totalHirelings();
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    auto quadsTape = tapes.hudQuads();
    test.startTaping();

    for (int actorId : {15, 1, 4}) { // Noreen the Banker, Tanya the Porter, Eleanor the Acolyte.
        talkToStreetNpc(game, actorId);
        clickNpcOption(game, "Join");
        clickNpcOption(game, "Hire");
    }

    auto hirelings = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position.y == 149; }); // Both portrait frames.
    EXPECT_EQ(hirelingsTape, tape(0, 1, 2));
    EXPECT_EQ(goldTape, tape(10000, 9000, 8900));
    EXPECT_EQ(statusTape, tape("Rest", "Noreen the Banker", "", "Tanya the Porter", "", "Eleanor the Acolyte", "", "I cannot join you, you're party is full"));
    const DrawnQuad noreen{"npc114", {491, 149}}, tanya{"npc250", {561, 149}};
    EXPECT_EQ(hirelings, tape(noreen, noreen, noreen, noreen, noreen, noreen, tanya, noreen, tanya, noreen, tanya, noreen, tanya, noreen, tanya));
}
