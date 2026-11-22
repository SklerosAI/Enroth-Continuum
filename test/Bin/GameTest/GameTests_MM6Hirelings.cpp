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
#include "Engine/TurnEngine/TurnEngine.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

#include "Utility/Math/TrigLut.h"

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

static void equipLongsword(int characterIndex) {
    auto &inventory = pParty->pCharacters[characterIndex].inventory;
    inventory.equip(ITEM_SLOT_MAIN_HAND, Item(static_cast<ItemId>(1))); // Longsword.
}

static void hireArmsMaster(EngineController &game) {
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    talkToStreetNpc(game, 8); // Chester the Arms Master.
    clickNpcOption(game, "Beg"); // A Paladin talks to a party of no reputation only once it begs.
    clickNpcOption(game, "Join");
    clickNpcOption(game, "Hire");
}

static void viewStats(EngineController &game) {
    // Open the active character's stats tab and close the character screen again.
    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick(2); // The key queues the character screen for the next frame.
    game.pressAndReleaseKey(PlatformKey::KEY_S);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

GAME_TEST(MM6Hirelings, StreetNpcPortraitExists) {
    // Talking to a street NPC drew a missing image for the portrait, it was one of MM7's elf portraits.
    startNewGameThroughInput(game);
    auto quadsTape = tapes.hudQuads();
    test.startTaping();

    talkToStreetNpc(game, 4); // Dal the Acolyte.

    auto portraits = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position == Pointi(525, 34); });
    EXPECT_EQ(portraits, tape(DrawnQuad{"npc197", {525, 34}}));
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

    for (int actorId : {15, 1, 4}) { // Noreen the Banker, Tanya the Porter, Dal the Acolyte.
        talkToStreetNpc(game, actorId);
        clickNpcOption(game, "Join");
        clickNpcOption(game, "Hire");
    }

    auto hirelings = quadsTape.flatten().filter([](const DrawnQuad &quad) { return quad.position.y == 149; }); // Both portrait frames.
    EXPECT_EQ(hirelingsTape, tape(0, 1, 2));
    EXPECT_EQ(goldTape, tape(10000, 9000, 8900));
    EXPECT_EQ(statusTape, tape("Rest", "Noreen the Banker", "", "Tanya the Porter", "", "Dal the Acolyte", "", "I cannot join you, you're party is full"));
    const DrawnQuad noreen{"npc114", {491, 149}}, tanya{"npc250", {561, 149}};
    EXPECT_EQ(hirelings, tape(noreen, noreen, noreen, noreen, noreen, noreen, tanya, noreen, tanya, noreen, tanya, noreen, tanya, noreen, tanya));
}

GAME_TEST(MM6Hirelings, ArmsMasterAddsToWeaponSkillAttack) {
    // A hired Arms Master didn't change the attack bonus on the stats screen. MM6 adds his two points to the weapon skill
    // of every character.
    startNewGameThroughInput(game);
    equipLongsword(0);
    auto textTape = tapes.drawnTexts();
    auto hirelingsTape = tapes.totalHirelings();
    test.startTaping();

    viewStats(game);
    hireArmsMaster(game);
    viewStats(game);

    auto rows = [&](std::string_view label) {
        return textTape.map([&](const AccessibleVector<DrawnText> &frame) {
            std::string result;
            for (const DrawnText &text : frame)
                if (text.text.starts_with(label))
                    result += text.text;
            return result;
        }).filter([](const std::string &row) { return !row.empty(); }).unique();
    };
    EXPECT_EQ(hirelingsTape, tape(0, 1));
    EXPECT_EQ(rows("Attack"), tape("Attack\t100+1\n", "Attack\t100+3\n"));
    EXPECT_EQ(rows("Damage"), tape("Damage\t100 5 - 11\nDamage\t100 N/A\n\n")); // A Novice sword adds no damage.
    EXPECT_EQ(rows("Armor Class"), tape("Armor Class\f65535\r4240\f00000 /\t1850\n")); // Only a Squire adds to armor skills.
}

GAME_TEST(MM6Hirelings, ArmsMasterShortensOnlyExpertSwordRecovery) {
    // A hired Arms Master cut the recovery of every melee attack by two, as MM7's Armsmaster skill does. MM6 adds his
    // points to the weapon skill, so only an Expert sword, axe or bow recovers faster. In turn-based mode the recovery is
    // the knight's wait for her next turn.
    struct Case {
        bool expert;
        bool hired;
        bool fix;
        int knightWait;
    };
    for (auto [expert, hired, fix, knightWait] : {Case{false, false, true, 89}, Case{false, true, true, 89}, Case{true, false, true, 88},
                                                  Case{true, true, true, 86}, Case{true, true, false, 88}}) {
        SCOPED_TRACE(fmt::format("expert={} hired={} fix={}", expert, hired, fix));
        test.prepareForNextTest();
        engine->config->gameplay.FixWeaponSkillHirelings.setValue(fix);
        startNewGameThroughInput(game);
        equipLongsword(0);
        if (expert)
            pParty->pCharacters[0].pActiveSkills[SKILL_SWORD] = CombinedSkillValue(1, MASTERY_EXPERT);
        if (hired)
            hireArmsMaster(game);
        const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
        const int goblin = game.spawnMonster(pParty->pos + ahead * 150, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
        auto hirelingsTape = tapes.totalHirelings();
        auto activeTape = tapes.activeCharacterIndex();
        auto knightWaitTape = tapes.custom([] {
            for (const TurnBased_QueueElem &element : pTurnEngine->pQueue)
                if (element.uPackedID == Pid::character(0))
                    return element.actor_initiative;
            return -1;
        });
        test.startTaping();

        game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
        game.tick();
        while (!(pTurnEngine->turn_stage == TE_ATTACK && pTurnEngine->flags & TE_PLAYER_TURN))
            game.tick(); // The monsters finish their opening move.
        game.pointMouseAtActor(goblin);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();

        EXPECT_EQ(hirelingsTape, tape(hired ? 1 : 0));
        EXPECT_EQ(activeTape.frontBack(), tape(-1, 2)); // The knight swings, then the turn passes on.
        EXPECT_EQ(knightWaitTape.back(), knightWait);
    }
}
