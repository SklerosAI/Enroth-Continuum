#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static constexpr QuestBit cubeQuest = static_cast<QuestBit>(166);
static constexpr QuestBit controlCenterAccess = static_cast<QuestBit>(169);
static constexpr QuestBit archibaldQuest = static_cast<QuestBit>(235);
static constexpr ItemId blaster = static_cast<ItemId>(64);
static constexpr ItemId controlCube = static_cast<ItemId>(456);

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

static void talkToOracle(EngineController &game) {
    // Space at the Oracle's screen, which the floor plate in front of her has raised.
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-1408, 2186, -496), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static std::vector<std::string> portraitLabels() {
    // Hints of the portraits the house screen lists.
    std::vector<std::string> result;
    if (window_SpeakInHouse)
        for (GUIButton *button : window_SpeakInHouse->vButtons)
            if (button->id.starts_with("House_Npc"))
                result.push_back(button->label);
    return result;
}

GAME_TEST(MM6ControlCenter, OracleTakesTheCube) {
    // The Oracle takes the Control Cube for the Control Center, and her Kreegan topic then opens her way down to it.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    const MapId sciFi = pMapTable->GetMapInfo("sci-fi.blv");
    teleportToMap(game, "oracle.blv", Vec3f(-1408, 2186, -496), 90);
    game.tick(); // A plate goes off only for a party that stood on another floor first.
    game.teleportTo(oracle, Vec3f(-1408, 1216, -512), 90); // Onto the floor plate in front of the Oracle.
    game.tick(4); // The plate starts raising the Oracle's screen out of the floor, and she can be talked to.
    pNPCStats->pNPCData[8].dialogue_1_evt_id = 76; // Her Control Center topic, which the four memory crystals give her.
    pNPCStats->pNPCData[8].dialogue_2_evt_id = 0;
    pParty->_questBits.set(cubeQuest); // Her quest for the cube.
    pParty->pCharacters[0].inventory.add(Item(controlCube));
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    auto portraitTape = tapes.custom([] { return portraitLabels(); });
    auto cubeQuestTape = tapes.questBit(cubeQuest);
    auto accessTape = tapes.questBit(controlCenterAccess);
    auto archibaldQuestTape = tapes.questBit(archibaldQuest);
    auto cubeTape = tapes.totalItemCount(controlCube);
    auto experienceTape = charTapes.experiences();
    auto awardTape = charTapes.custom([](const Character &character) { return character._achievedAwardsBits[static_cast<AwardId>(34)]; });
    auto topicsTape = tapes.custom([] { return std::vector<unsigned>{pNPCStats->pNPCData[8].dialogue_1_evt_id, pNPCStats->pNPCData[8].dialogue_2_evt_id}; });
    auto nicolaiTape = tapes.custom([] { return pNPCStats->pNPCData[13].dialogue_2_evt_id; });
    test.startTaping();

    talkToOracle(game);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 76));
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 77));
    leaveMM6CampaignChainHouse(game);
    talkToOracle(game);
    game.pressGuiButton("House_Npc1");
    game.tick();
    game.pressGuiButton("HouseExit_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();

    EXPECT_EQ(mapTape, tape(oracle, sciFi));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(170), HOUSE_INVALID, static_cast<HouseId>(170), HOUSE_INVALID));
    EXPECT_EQ(cubeQuestTape, tape(true, false));
    EXPECT_EQ(cubeTape, tape(1, 0));
    EXPECT_EQ(accessTape, tape(false, true));
    EXPECT_EQ(archibaldQuestTape, tape(false, true));
    EXPECT_EQ(topicsTape, tape(std::vector<unsigned>{76, 0}, std::vector<unsigned>{77, 78}));
    EXPECT_EQ(nicolaiTape.back(), 26); // His Third Eye topic.
    EXPECT_EQ(awardTape, tape({false, false, false, false}, {true, true, true, true}));
    for (int i = 0; i < 4; i++)
        EXPECT_EQ(experienceTape.back()[i] - experienceTape.front()[i], 500000);
    EXPECT_CONTAINS(portraitTape, std::vector<std::string>({localization->format(LSTR_CONVERSE_WITH_S, "Oracle"),
                                                            localization->format(LSTR_ENTER_S, pMapTable->pInfos[sciFi].name)}));
}

GAME_TEST(MM6ControlCenter, BlasterInStorageContainer) {
    // The Control Center's storage containers hold its blasters. The one by the party start is trapped, the first click
    // sets the trap off and the second opens it.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    teleportToMap(game, "sci-fi.blv", Vec3f(-9280, 4160, 196), 0);
    engine->config->debug.NoDamage.setValue(true); // A new party dies to the trap.
    auto screenTape = tapes.screen();
    auto blasterTape = charTapes.haveItem(blaster);
    auto chestTape = tapes.custom([] { return static_cast<bool>(vChests[1].inventory.find(blaster)); });
    test.startTaping();

    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-11162, 2912, 196), 180, -20); // In front of the container.
    game.pointMouseAtFace(2791);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick(4); // The trap's flash burns out.
    game.pointMouseAtFace(2791);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    takeMM6CampaignChainChestItem(game, 1, blaster);

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST, SCREEN_GAME));
    EXPECT_EQ(chestTape, tape(true, false));
    EXPECT_EQ(blasterTape, tape({false, false, false, false}, {true, false, false, false}));
}
