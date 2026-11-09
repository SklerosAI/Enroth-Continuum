#include <algorithm>
#include <cstdio>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Interaction.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "GUI/UI/UIStatusBar.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIGameOver.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"
#include "Engine/Graphics/LocationFunctions.h"

static void dumpFaces(EngineController &game, MapId map, std::vector<int> events) {
    game.teleportTo(map, Vec3f(0, 0, 0), 0);
    std::vector<BLVFace> &faces = uCurrentlyLoadedLevelType == LEVEL_INDOOR ? pIndoor->faces : pOutdoor->faces;
    for (const BLVFace &face : faces) {
        for (int e : events) {
            if (face.eventId != e)
                continue;
            const BBoxf &b = face.boundingBox;
            std::printf("PROBE map=%d event=%d face=%d bbox=(%.0f..%.0f, %.0f..%.0f, %.0f..%.0f) n=(%.2f,%.2f,%.2f)\n",
                        static_cast<int>(map), e, face.faceId, b.x1, b.x2, b.y1, b.y2, b.z1, b.z2,
                        face.facePlane.normal.x, face.facePlane.normal.y, face.facePlane.normal.z);
        }
    }
    for (size_t i = 0; i < pLevelDecorations.size(); i++)
        for (int e : events)
            if (pLevelDecorations[i].uEventID == e)
                std::printf("PROBE map=%d event=%d deco=%zu pos=(%.0f,%.0f,%.0f)\n", static_cast<int>(map), e, i,
                            pLevelDecorations[i].vPosition.x, pLevelDecorations[i].vPosition.y, pLevelDecorations[i].vPosition.z);
    for (size_t i = 0; i < pActors.size(); i++) {
        if (pActors[i].group == 9)
            std::printf("PROBE map=%d actor=%zu group9 pos=(%.0f,%.0f,%.0f) state=%d\n", static_cast<int>(map), i,
                        pActors[i].pos.x, pActors[i].pos.y, pActors[i].pos.z, static_cast<int>(pActors[i].aiState));
    }
}

GAME_TEST(MM7Campaign, ArbiterChoice) {
    for (bool light : {true, false}) {
        SCOPED_TRACE(fmt::format("light={}", light));
        test.prepareForNextTest();
        auto alignmentTape = tapes.custom([] { return pParty->alignment; });
        auto lightTape = tapes.questBit(QBIT_LIGHT_PATH);
        auto darkTape = tapes.questBit(QBIT_DARK_PATH);
        auto archibaldTape = tapes.questBit(QBIT_ARCHIBALD_VISIT_ACTIVE);
        auto gavinTape = tapes.questBit(QBIT_GAVIN_MAGNUS_VISIT_ACTIVE);
        auto choiceTape = tapes.questBit(QBIT_ARBITER_CHOICE_ACTIVE);
        auto hirelingsTape = tapes.totalHirelings();
        auto houseTape = tapes.house();
        auto texturesTape = tapes.hudTextures();
        game.startNewGame();
        pParty->_questBits.set(QBIT_ARBITER_MESSENGER_MET);
        pParty->_questBits.set(QBIT_ARBITER_CHOICE_ACTIVE);
        pNPCStats->pNPCData[77].house = HOUSE_TAVERN_BRACADA_DESERT;
        pNPCStats->pNPCData[78].house = HOUSE_TAVERN_DEYJA;
        test.startTaping();
        if (light) {
            game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0);
        } else {
            game.teleportTo(MAP_DEYJA, Vec3f(4544, -12350, 2592), 90);
        }
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        game.pressGuiButton("House_Npc1"); // The judge.
        game.tick();
        game.pressGuiButton("HouseNpcDialogue_Option0");
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90); // In front of the arbiter's door.
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();

        auto setOnlyIf = [](bool condition) { return condition ? tape(false, true) : tape(false); };
        EXPECT_EQ(alignmentTape, tape(PartyAlignment::PartyAlignment_Neutral, light ? PartyAlignment::PartyAlignment_Good : PartyAlignment::PartyAlignment_Evil));
        EXPECT_EQ(lightTape, setOnlyIf(light));
        EXPECT_EQ(darkTape, setOnlyIf(!light));
        EXPECT_EQ(gavinTape, setOnlyIf(light));
        EXPECT_EQ(archibaldTape, setOnlyIf(!light));
        EXPECT_EQ(choiceTape, tape(true, false));
        EXPECT_EQ(hirelingsTape, tape(1, 2, 1)); // The arbiter takes the judge back.
        EXPECT_EQ(houseTape, light ? tape(HOUSE_INVALID, HOUSE_TAVERN_BRACADA_DESERT, HOUSE_INVALID, HOUSE_HARMONDALE_ARBITER_1)
                                   : tape(HOUSE_INVALID, HOUSE_TAVERN_DEYJA, HOUSE_INVALID, HOUSE_HARMONDALE_ARBITER_2));
        EXPECT_CONTAINS(texturesTape.flatten(), light ? "ib-r-b.pcx" : "ib-r-c.pcx"); // The HUD frame matches the side.
        EXPECT_MISSES(texturesTape.flatten(), light ? "ib-r-c.pcx" : "ib-r-b.pcx");
    }
}

GAME_TEST(MM7Campaign, Endings) {
    for (bool light : {true, false}) {
        SCOPED_TRACE(fmt::format("light={}", light));
        test.prepareForNextTest();
        auto screenTape = tapes.screen();
        auto mapTape = tapes.map();
        auto houseTape = tapes.house();
        auto autosaveTape = tapes.custom([] { return ufs->exists("saves/autosave.mm7"); });
        auto overthrusterTape = tapes.hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
        auto activeTape = tapes.questBit(light ? QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE : QBIT_OSCILLATION_OVERTHRUSTER_DARK_ACTIVE);
        auto certificateTape = tapes.custom([] { return ufs->exists("MM7_Win.Pcx"); });
        game.startNewGame();
        pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_OSCILLATION_OVERTHRUSTER));
        if (light) {
            pParty->_questBits.set(QBIT_LIGHT_PATH);
            pNPCStats->pNPCData[80].house = HOUSE_CASTLE_LAMBENT_THRONE_ROOM; // Resurectra.
            pNPCStats->pNPCData[80].dialogue_2_evt_id = 169;
            game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90); // In front of the throne room door.
        } else {
            pParty->_questBits.set(QBIT_DARK_PATH);
            pParty->_questBits.set(QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB); // Kastore's throne room is behind the door.
            pNPCStats->pNPCData[84].house = HOUSE_CASTLE_GLOAMING_THRONE_ROOM_2; // Kastore.
            pNPCStats->pNPCData[84].dialogue_2_evt_id = 171;
            engine->config->debug.NoActors.setValue(true); // Monsters nearby would stop the party from resting.
            game.pressAndReleaseKey(PlatformKey::KEY_R);
            game.tick();
            game.pressGuiButton("Rest_WaitTillDawn"); // The throne room only opens at night.
            game.tick();
            while (currentRestType == REST_WAIT)
                game.tick();
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
            game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(-10300, -18432, -2861), 180); // In front of the throne room door.
        }
        ufs->remove("saves/autosave.mm7");
        ufs->remove("MM7_Win.Pcx");
        test.startTaping();
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (light) {
            game.pressGuiButton("House_Npc1"); // Resurectra. Kastore is alone in his throne room.
            game.tick();
        }
        game.pressGuiButton("HouseNpcDialogue_Option0"); // The final task.
        game.tick();
        game.pressGuiButton("HouseNpcDialogue_Option0"); // Hand over the overthruster.
        game.tick(2); // The game over window opens on the frame after the topic.
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE); // Shows the farewell box.
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick(2); // The window closes, then the party is sent to Harmondale.

        EXPECT_EQ(screenTape, tape(SCREEN_HOUSE, SCREEN_GAMEOVER_WINDOW, SCREEN_GAME));
        EXPECT_EQ(houseTape, light ? tape(HOUSE_CASTLE_LAMBENT_THRONE_ROOM, HOUSE_INVALID)
                                   : tape(HOUSE_CASTLE_GLOAMING_THRONE_ROOM_2, HOUSE_INVALID));
        EXPECT_EQ(mapTape, tape(light ? MAP_CASTLE_LAMBENT : MAP_CASTLE_GLOAMING, MAP_HARMONDALE));
        EXPECT_EQ(activeTape, tape(false, true));
        EXPECT_EQ(overthrusterTape, tape(true, false));
        EXPECT_EQ(certificateTape, tape(false, true));
        EXPECT_EQ(autosaveTape, tape(false, true));
        EXPECT_EQ(pParty->pos, Vec3f(-17331, 12547, 465));
    }
}

GAME_TEST(MM7Campaign, ArbiterMessenger) {
    // The arbiter's messenger should come on a Harmondale visit 112 days after the castle is rebuilt, and not before.
    for (int dawns : {112, 113}) {
        SCOPED_TRACE(fmt::format("dawns={}", dawns));
        test.prepareForNextTest();
        auto messengerTape = tapes.questBit(QBIT_ARBITER_MESSENGER_MET);
        auto choiceTape = tapes.questBit(QBIT_ARBITER_CHOICE_ACTIVE);
        auto screenTape = tapes.screen();
        auto mapTape = tapes.map();
        auto judgesTape = tapes.custom([] { return std::pair(pNPCStats->pNPCData[77].house, pNPCStats->pNPCData[78].house); });
        engine->config->debug.NoActors.setValue(true); // Monsters nearby would stop the party from resting.
        game.startNewGame();
        pParty->_questBits.set(QBIT_HARMONDALE_REBUILT);
        pParty->SetFood(1000);
        game.teleportTo(MAP_HARMONDALE, Vec3f(-17331, 12547, 465), 0);
        test.startTaping();
        for (int i = 0; i < dawns; i++) {
            for (std::string_view button : {"Rest_RestAndHeal", "Rest_WaitTillDawn"}) {
                game.pressAndReleaseKey(PlatformKey::KEY_R);
                game.tick();
                game.pressGuiButton(button);
                game.tick();
                while (currentRestType != REST_NONE)
                    game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE); // The rest screen stays open after a wait.
            game.tick();
        }
        game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(12552, 800, 193), 0);
        game.teleportTo(MAP_HARMONDALE, Vec3f(-17331, 12547, 465), 0);

        bool arrived = dawns == 113; // A new game starts at 9am and each day of rest ends at 5am, so 112 days is 2684 hours.
        EXPECT_EQ(messengerTape, arrived ? tape(false, true) : tape(false));
        EXPECT_EQ(choiceTape, arrived ? tape(false, true) : tape(false));
        EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_EMERALD_ISLAND, MAP_HARMONDALE));
        EXPECT_EQ(screenTape.back(), arrived ? SCREEN_NPC_DIALOGUE : SCREEN_GAME);
        auto judgesAway = std::pair(HOUSE_INVALID, HOUSE_INVALID);
        auto judgesInTaverns = std::pair(HOUSE_TAVERN_BRACADA_DESERT, HOUSE_TAVERN_DEYJA);
        EXPECT_EQ(judgesTape, arrived ? tape(judgesAway, judgesInTaverns) : tape(judgesAway));
    }
}

static void killGroup(int group, int survivor) {
    for (size_t i = 0; i < pActors.size(); i++)
        if (pActors[i].group == group && static_cast<int>(i) != survivor)
            pActors[i].aiState = Dead;
}

GAME_TEST(MM7Campaign, DISABLED_HarmondaleRaid) {
    // Killing every raider in Castle Harmondale and in Harmondale should end the raid and open the town's shops again.
    for (bool survivor : {false, true}) {
        SCOPED_TRACE(fmt::format("survivor={}", survivor));
        test.prepareForNextTest();
        auto raidTape = tapes.questBit(QBIT_MERCENARY_GUILD_QUEST_FAILED);
        auto castleTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_RAIDERS_KILLED);
        auto townTape = tapes.questBit(QBIT_HARMONDALE_RAIDERS_KILLED);
        auto finishedTape = tapes.questBit(QBIT_MALWICK_QUESTS_FINISHED);
        auto houseTape = tapes.house();
        auto statusTape = tapes.statusBar();
        game.startNewGame();
        pParty->_questBits.set(QBIT_MALWICK_FIREBALL_WAND_ACCEPTED);
        pParty->_questBits.set(QBIT_MERCENARY_GUILD_QUEST_FAILED);
        test.startTaping();
        game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(-3770, 1664, 0), 0);
        std::printf("DIAG before kill actors=%zu map=%d g9=%d\n", pActors.size(), (int)engine->_currentLoadedMapId, (int)std::count_if(pActors.begin(), pActors.end(), [](Actor &a) { return a.group == 9; }));
        killGroup(9, -1);
        int castleTicks = 0;
        while (!pParty->_questBits[QBIT_CASTLE_HARMONDALE_RAIDERS_KILLED] && castleTicks < 2000) {
            game.tick();
            castleTicks++;
        }
        {
            int total = 0, dead = 0;
            for (Actor &a : pActors) if (a.group == 9) { total++; if (a.IsNotAlive()) dead++; else std::printf("DIAG alive state=%d attr=%x\n", (int)a.aiState, (unsigned)a.attributes); }
            std::printf("DIAG d29 total=%d dead=%d alert=%d\n", total, dead, (int)GetAlertStatus());
        }
        game.teleportTo(MAP_HARMONDALE, Vec3f(-6472, 14750, 32), 90); // In front of the weapon shop door.
        killGroup(9, survivor ? 9 : -1);
        int townTicks = 0;
        while (!pParty->_questBits[QBIT_HARMONDALE_RAIDERS_KILLED] && townTicks < 2000) {
            game.tick();
            townTicks++;
        }
        std::printf("DIAG castleTicks=%d townTicks=%d\n", castleTicks, townTicks);
        game.teleportTo(MAP_HARMONDALE, Vec3f(-6472, 14750, 32), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        std::printf("DIAG house=%d status=%s\n", window_SpeakInHouse ? (int)window_SpeakInHouse->houseId() : -1, engine->_statusBar->get().c_str());

        EXPECT_EQ(raidTape, survivor ? tape(true) : tape(true, false));
        EXPECT_EQ(castleTape, tape(false, true));
        EXPECT_EQ(townTape, survivor ? tape(false) : tape(false, true));
        EXPECT_EQ(finishedTape, survivor ? tape(false) : tape(false, true));
    }
}

GAME_TEST(MM7Campaign, AliceHargreavesHouse) {
    // Alice Hargreaves' room in Castle Gryphonheart should open even though the game has no video for it.
    game.startNewGame();
    engine->config->debug.NoVideo.setValue(false);
    pParty->_questBits.set(QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE);
    game.teleportTo(MAP_CASTLE_GRYPHONHEART, Vec3f(-6752, -4750, 1387), 270); // In front of her door.
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_NE(window_SpeakInHouse, nullptr);
    EXPECT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES);
}

GAME_TEST(MM7Campaign, LootDoesNotCompleteHarmondaleRaid) {
    game.startNewGame();
    for (ItemId item : {ITEM_ARTIFACT_SPLITTER, ITEM_SPELLBOOK_REMOVE_FEAR}) {
        pSpriteObjects.clear();
        SpriteObject sprite;
        sprite.containing_item = Item(item);
        pSpriteObjects.push_back(sprite);
        game.runGameRoutine([] { ItemInteraction(0); });
        EXPECT_FALSE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_RAIDERS_KILLED]);
        EXPECT_FALSE(pParty->_questBits[QBIT_HARMONDALE_RAIDERS_KILLED]);
    }
}
