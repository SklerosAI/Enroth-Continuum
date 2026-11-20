#include <algorithm>
#include <initializer_list>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static std::vector<int> archibaldBits() {
    // The set ones of the Third Eye found and asked for, the Archibald quest and his release, the bell carried, the
    // Control Center's access and its terminal used.
    std::vector<int> result;
    for (int bit : {96, 97, 169, 177, 196, 197, 198, 235, 276, 305})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static void putHeldItemIntoBackpack(EngineController &game) {
    ASSERT_NE(pParty->pPickedItem.itemId, ITEM_NULL);
    game.pressGuiButton("Game_Character1");
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
}

static void takeBlasters(EngineController &game, int chestEvent, int chestId) {
    // Every blaster in a storage container. Taking an item closes the container, so it opens again for the next one.
    for (int attempt = 0; attempt < 3 && current_screen_type != SCREEN_CHEST; attempt++)
        clickMM6CampaignChainIndoorEventFace(game, chestEvent); // A trap that goes off keeps the container shut for that click.
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (ItemId blaster : {static_cast<ItemId>(64), static_cast<ItemId>(65)}) {
        while (vChests[chestId].inventory.find(blaster)) {
            takeMM6CampaignChainChestItem(game, chestId, blaster);
            if (vChests[chestId].inventory.find(static_cast<ItemId>(64)) || vChests[chestId].inventory.find(static_cast<ItemId>(65)))
                clickMM6CampaignChainIndoorEventFace(game, chestEvent);
        }
    }
}

static void visitControlCenter(EngineController &game) {
    // Down to the Oracle, through her way to the Control Center, the terminal's Blaster lesson and the storage
    // containers' blasters, then back up.
    const MapId sciFi = pMapTable->GetMapInfo("sci-fi.blv");
    enterMM6CampaignChainOracle(game);
    useMM6CampaignChainIndoorEventFace(game, 3); // The Oracle. Her screen is still raised from C5.
    takeMM6CampaignChainHouseExit(game, sciFi);
    useMM6CampaignChainIndoorEventFace(game, 61); // The terminal.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE); // Any key closes the terminal's text.
    game.tick();
    takeBlasters(game, 31, 2);
    takeBlasters(game, 30, 1);
    useMM6CampaignChainIndoorEventFace(game, 70); // The way back to the Oracle.
    confirmMM6CampaignChainTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oracle.blv"));
    useMM6CampaignChainIndoorEventFace(game, 50); // The way out.
    confirmMM6CampaignChainTransition(game);
}

static void askNicolai(EngineController &game, int topic) {
    // Castle Ironfist's door leads to the throne room, where Prince Nicolai sits.
    enterMM6CampaignChainCastle(game, pMapTable->GetMapInfo("outd3.odm"), 43);
    talkToMM6CampaignChainNpc(game, 13);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, topic));
    leaveMM6CampaignChainHouse(game);
}

static void searchWell(EngineController &game) {
    // The well by the castle has its event on the water, which Space reaches from beside the rim.
    auto face = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 231; });
    ASSERT_NE(face, pOutdoor->faces.end());
    game.teleportTo(engine->_currentLoadedMapId, face->boundingBox.center() - Vec3f(0, 150, 0), 90);
    game.tick(); // The party lands next to the well.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void releaseArchibald(EngineController &game) {
    // The library door with Tanir's Bell plays the bell's movie, which leads to Archibald's room. His topic hands over
    // the Ritual of the Void and empties the room.
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("outd3.odm"), 42);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    for (int frame = 0; frame < 50 && window_SpeakInHouse->houseId() != static_cast<HouseId>(553); frame++)
        game.tick(); // The bell's movie plays, and Archibald wakes.
    ASSERT_EQ(window_SpeakInHouse->houseId(), static_cast<HouseId>(553));
    talkToMM6CampaignChainNpc(game, 12);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 30));
    for (int frame = 0; frame < 10 && window_SpeakInHouse->houseId() != static_cast<HouseId>(554); frame++)
        game.tick(); // His room plays out its last pass.
    ASSERT_EQ(window_SpeakInHouse->houseId(), static_cast<HouseId>(554));
    leaveMM6CampaignChainHouse(game);
    putHeldItemIntoBackpack(game); // The Ritual of the Void.
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter8) {
    // Fixtures: C1's party strength. Teleports to the High Council door on outc2, inside the Oracle to the Oracle and
    // the way out, inside the Control Center to the terminal, two storage containers and the way back, and on outd3
    // to Castle Ironfist's door, beside the well and to the King's Library door.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 7);
    engine->config->debug.NoVideo.setValue(false); // The King's Library shows its rooms through movies.
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    const MapId sciFi = pMapTable->GetMapInfo("sci-fi.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, outc2);

    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    auto bitsTape = tapes.custom([] { return archibaldBits(); });
    auto blasterSkillTape = charTapes.skillLevels(SKILL_BLASTER);
    auto blasterTape = tapes.totalItemCount(static_cast<ItemId>(64));
    auto rifleTape = tapes.totalItemCount(static_cast<ItemId>(65));
    auto thirdEyeTape = tapes.totalItemCount(static_cast<ItemId>(446));
    auto bellTape = tapes.totalItemCount(static_cast<ItemId>(461));
    auto ritualTape = tapes.totalItemCount(static_cast<ItemId>(544));
    auto nicolaiTopicTape = tapes.custom([] { return pNPCStats->pNPCData[13].dialogue_2_evt_id; });
    auto oracleTopicsTape = tapes.custom([] {
        return std::vector<unsigned>{pNPCStats->pNPCData[8].dialogue_1_evt_id, pNPCStats->pNPCData[8].dialogue_2_evt_id};
    });
    auto archibaldHouseTape = tapes.custom([] { return pNPCStats->pNPCData[12].house; });
    auto experienceTape = tapes.totalExperience();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    visitControlCenter(game);
    askNicolai(game, 26); // The Third Eye.
    searchWell(game);
    putHeldItemIntoBackpack(game); // The Third Eye.
    askNicolai(game, 27); // The Third Eye opens the family vault, and he brings Tanir's Bell from it.
    putHeldItemIntoBackpack(game); // Tanir's Bell.
    releaseArchibald(game);

    EXPECT_EQ(mapTape, tape(outc2, oracle, sciFi, oracle, outc2, outd3));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(165), HOUSE_INVALID, static_cast<HouseId>(170), HOUSE_INVALID,
                              static_cast<HouseId>(154), HOUSE_INVALID, static_cast<HouseId>(154), HOUSE_INVALID, static_cast<HouseId>(168),
                              static_cast<HouseId>(553), static_cast<HouseId>(554), HOUSE_INVALID));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{169, 235}, std::vector<int>{169, 235, 276}, std::vector<int>{96, 169, 235, 276},
                             std::vector<int>{96, 169, 196, 235, 276, 305}, std::vector<int>{169, 197, 235, 276, 305},
                             std::vector<int>{169, 177, 198, 276, 305}));
    EXPECT_EQ(blasterSkillTape, tape({0, 0, 0, 0}, {1, 1, 1, 1}));
    EXPECT_EQ(blasterTape, tape(0, 1, 2, 3));
    EXPECT_EQ(rifleTape, tape(0, 1));
    EXPECT_EQ(thirdEyeTape, tape(0, 1, 0));
    EXPECT_EQ(bellTape, tape(0, 1));
    EXPECT_EQ(ritualTape, tape(0, 1));
    EXPECT_EQ(nicolaiTopicTape, tape(26u, 27u, 28u));
    EXPECT_EQ(oracleTopicsTape, tape(std::vector<unsigned>{77, 78}, std::vector<unsigned>{77, 0}));
    EXPECT_EQ(archibaldHouseTape, tape(static_cast<HouseId>(553), HOUSE_INVALID));
    EXPECT_EQ(experienceTape.delta(), 200000);
    EXPECT_EQ(reputationTape, tape(1600, 672)); // The release's Subtract 1000 keeps its low byte, 232, once for each character.
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 8);
}
