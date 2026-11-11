#include <algorithm>
#include <array>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

static DoorState wallsDoorState(int doorId) {
    auto door = std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId);
    return door == pIndoor->doors.end() ? DOOR_CLOSED : door->state;
}

GAME_TEST(MM7CampaignChain, DISABLED_Chapter4) {
    // Fixtures: teleports to Bracada Desert and onto its Celeste portal, to the Castle Lambent throne room door, to the
    // Celeste town portal arrival point, and to the levers, chests and pedestals in the Walls of Mist.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save004.mm7"));
    loadCampaignChainSlot(game, 3);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);

    auto activeTape = tapes.questBit(QBIT_WALLS_OF_MIST_ACTIVE);
    auto passedTape = tapes.questBit(QBIT_WALLS_OF_MIST_PASSED);
    auto guildTape = tapes.questBit(QBIT_LIGHT_GUILD_JOINED);
    auto westTape = tapes.hasItem(ITEM_WEST_PILLAR_KEY);
    auto centralTape = tapes.hasItem(ITEM_CENTRAL_PILLAR_KEY);
    auto eastTape = tapes.hasItem(ITEM_EAST_PILLAR_KEY);
    auto advisorHousesTape = tapes.custom([] {
        return std::array{pNPCStats->pNPCData[80].house, pNPCStats->pNPCData[81].house,
                          pNPCStats->pNPCData[82].house, pNPCStats->pNPCData[83].house};
    });
    auto advisorTopicsTape = tapes.custom([] {
        return std::array{pNPCStats->pNPCData[80].dialogue_1_evt_id, pNPCStats->pNPCData[81].dialogue_1_evt_id,
                          pNPCStats->pNPCData[82].dialogue_1_evt_id, pNPCStats->pNPCData[83].dialogue_1_evt_id};
    });
    auto throneTopicTape = tapes.custom([] { return pNPCStats->pNPCData[79].dialogue_1_evt_id; });
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();

    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0);
    auto portal = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 503; });
    ASSERT_NE(portal, pOutdoor->faces.end());
    Vec3f portalPosition = portal->boundingBox.center();
    portalPosition.z = portal->boundingBox.z1 + 50;
    game.teleportTo(MAP_BRACADA_DESERT, portalPosition, 0);
    for (int i = 0; i < 10 && current_screen_type != SCREEN_INPUT_BLV; i++) // The party drops onto the portal floor.
        game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);

    useCampaignChainTransition(game, MAP_CELESTE, 503);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    openCampaignChainThroneRoom(game, 79);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 79, 145));
    leaveCampaignChainHouse(game);

    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0); // Town portal arrival point.
    useCampaignChainTransition(game, MAP_CELESTE, 502);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WALLS_OF_MIST);
    for (int lever : {5, 8})
        ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_WALLS_OF_MIST, lever, 1, [lever] { return engine->_persistentVariables.mapVars[lever] == 1; }));
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_WALLS_OF_MIST, 10, 1, [] { return wallsDoorState(4) != DOOR_CLOSED; })); // The button slides the floor away from the first chest.
    for (int i = 0; i < 100 && wallsDoorState(4) != DOOR_OPEN; i++)
        game.tick();
    ASSERT_EQ(wallsDoorState(4), DOOR_OPEN);
    constexpr std::array keys = {ITEM_WEST_PILLAR_KEY, ITEM_CENTRAL_PILLAR_KEY, ITEM_EAST_PILLAR_KEY};
    constexpr std::array chestEvents = {177, 178, 179};
    for (int i = 0; i < 3; i++)
        lootCampaignChainChest(game, MAP_WALLS_OF_MIST, chestEvents[i], keys[i]);
    constexpr std::array pedestalFaces = {164, 4019, 4075};
    constexpr std::array pedestalPositions = {Vec3f(-1122, 3392, 1), Vec3f(-482, 3264, 1), Vec3f(158, 3392, 1)};
    for (int i = 0; i < 3; i++) {
        for (int attempt = 0; attempt < 10 && pParty->hasItem(keys[i]); attempt++) {
            game.teleportTo(MAP_WALLS_OF_MIST, pedestalPositions[i], 180, -22); // Looking down at the pedestal.
            try {
                game.pointMouseAtFace(pedestalFaces[i]);
            } catch (const Exception &) {
                game.tick();
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
        }
        ASSERT_FALSE(pParty->hasItem(keys[i]));
    }
    for (int i = 0; i < 100 && wallsDoorState(1) != DOOR_OPEN; i++) // The exit doors slide open.
        game.tick();
    ASSERT_EQ(wallsDoorState(1), DOOR_OPEN);
    useCampaignChainTransition(game, MAP_WALLS_OF_MIST, 502);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);

    openCampaignChainThroneRoom(game, 79);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 79, 146));
    leaveCampaignChainHouse(game);

    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(passedTape, tape(false, true));
    EXPECT_EQ(guildTape, tape(false, true));
    EXPECT_EQ(westTape, tape(false, true, false));
    EXPECT_EQ(centralTape, tape(false, true, false));
    EXPECT_EQ(eastTape, tape(false, true, false));
    EXPECT_EQ(advisorHousesTape, tape(std::array{HOUSE_INVALID, HOUSE_INVALID, HOUSE_INVALID, HOUSE_INVALID},
                                      std::array{HOUSE_CELESTE_HOSTEL_1, HOUSE_CELESTE_HOSTEL_2,
                                                 HOUSE_CELESTE_HOSTEL_3, HOUSE_CELESTE_HOSTEL_4}));
    EXPECT_EQ(advisorTopicsTape, tape(std::array{147u, 151u, 153u, 155u}));
    EXPECT_EQ(throneTopicTape, tape(145u, 146u, 0u));
    EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_BRACADA_DESERT, MAP_CELESTE, MAP_CASTLE_LAMBENT, MAP_CELESTE,
                            MAP_WALLS_OF_MIST, MAP_CELESTE, MAP_CASTLE_LAMBENT));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_CASTLE_LAMBENT_THRONE_ROOM, HOUSE_INVALID,
                              HOUSE_CASTLE_LAMBENT_THRONE_ROOM, HOUSE_INVALID));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 4);
}
