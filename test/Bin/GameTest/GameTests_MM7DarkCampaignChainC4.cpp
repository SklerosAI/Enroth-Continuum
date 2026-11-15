#include <algorithm>
#include <array>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"
#include "Utility/String/Ascii.h"

#include "MM7CampaignChain.h"

static std::vector<int> breedingZoneCompletionEvents() {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        const auto &instructions = engine->_localEventMap.function(face.eventId);
        const bool completesQuest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
            return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(QBIT_BREEDING_ZONE_FINISHED);
        });
        if (completesQuest && std::ranges::find(result, face.eventId) == result.end())
            result.push_back(face.eventId);
    }
    return result;
}

static std::vector<int> mapTransitionEventsTo(MapId map) {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        const auto &instructions = engine->_localEventMap.function(face.eventId);
        const bool movesToMap = std::ranges::any_of(instructions, [map](const EvtInstruction &instruction) {
            return instruction.opcode == EVENT_MoveToMap &&
                   ascii::noCaseEquals(instruction.str, pMapTable->pInfos[map].fileName);
        });
        if (movesToMap && std::ranges::find(result, face.eventId) == result.end())
            result.push_back(face.eventId);
    }
    return result;
}

static bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

static void openArchibaldThrone(EngineController &game) {
    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(-10300, -18432, -2861), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_GLOAMING_THRONE_ROOM_1);
    ASSERT_EQ(houseNpcs.size(), 1u);
}

static void visitPitAdviser(EngineController &game, int npcId) {
    const HouseId houseId = pNPCStats->pNPCData[npcId].house;
    ASSERT_NE(houseId, HOUSE_INVALID);
    auto door = std::ranges::find_if(pIndoor->faces, [houseId](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, houseId);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachCampaignChainFace(*door);
    game.teleportTo(MAP_PIT, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    leaveCampaignChainHouse(game);
}

GAME_TEST(MM7DarkCampaignChain, DISABLED_Chapter4) {
    // Fixtures: actors are disabled while resting and traversing the Breeding Zone. Teleports approach the Castle
    // Gloaming throne, the Breeding Zone interaction, and each Pit adviser door; the quest events, dungeon exit,
    // dialogue hand-in, house entries and save all use their production interaction paths.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save004.mm7"));
    loadCampaignChainSlot(game, 3);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_DARK_PATH]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ARCHIBALD_VISIT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_BREEDING_ZONE_ACTIVE]);

    auto activeTape = tapes.questBit(QBIT_BREEDING_ZONE_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_BREEDING_ZONE_FINISHED);
    auto guildTape = tapes.questBit(QBIT_DARK_GUILD_JOINED);
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();

    engine->config->debug.NoActors.setValue(true);
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    while (currentRestType == REST_WAIT)
        game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    openArchibaldThrone(game);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_BREEDING_ZONE_ACTIVE]);
    leaveCampaignChainHouse(game);

    game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
    const std::vector<int> completionEvents = breedingZoneCompletionEvents();
    ASSERT_EQ(completionEvents.size(), 1u);
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_BREEDING_ZONE, completionEvents.front(), 1, [] {
        return pParty->_questBits[QBIT_BREEDING_ZONE_FINISHED];
    }));
    const std::vector<int> exitEvents = mapTransitionEventsTo(MAP_PIT);
    ASSERT_EQ(exitEvents.size(), 2u); // The two authored dungeon exits both return to the Pit.
    bool exitOpened = false;
    for (int eventId : exitEvents) {
        if (clickCampaignChainEventFace(game, MAP_BREEDING_ZONE, eventId, 1, [] {
                return current_screen_type == SCREEN_INPUT_BLV;
            })) {
            exitOpened = true;
            break;
        }
    }
    ASSERT_TRUE(exitOpened);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_PIT);

    openArchibaldThrone(game);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_DARK_GUILD_JOINED]);
    leaveCampaignChainHouse(game);

    game.teleportTo(MAP_PIT, Vec3f(), 0);
    const std::array adviserHouses = {pNPCStats->pNPCData[84].house, pNPCStats->pNPCData[85].house,
                                      pNPCStats->pNPCData[86].house};
    EXPECT_NE(adviserHouses[0], adviserHouses[1]);
    EXPECT_NE(adviserHouses[0], adviserHouses[2]);
    EXPECT_NE(adviserHouses[1], adviserHouses[2]);
    for (int npcId : std::array{84, 85, 86})
        visitPitAdviser(game, npcId);
    engine->config->debug.NoActors.setValue(false);

    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(guildTape, tape(false, true));
    EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_CASTLE_GLOAMING, MAP_BREEDING_ZONE, MAP_PIT,
                            MAP_CASTLE_GLOAMING, MAP_PIT));
    EXPECT_EQ(houseTape.size(), 11u); // Invalid/throne/invalid twice, then three adviser house/invalid pairs.
    EXPECT_EQ(houseTape.front(), HOUSE_INVALID);
    EXPECT_EQ(houseTape.back(), HOUSE_INVALID);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 4);
}
