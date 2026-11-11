#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"

#include "MM7CampaignChain.h"

static std::vector<int> breedingZoneCompletionEvents() {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        const auto &instructions = engine->_localEventMap.function(face.eventId);
        bool completesQuest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
            return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(QBIT_BREEDING_ZONE_FINISHED);
        });
        if (completesQuest && std::ranges::find(result, face.eventId) == result.end())
            result.push_back(face.eventId);
    }
    return result;
}

static std::vector<int> clankerDefenseEvents() {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        const auto &instructions = engine->_localEventMap.function(face.eventId);
        bool disablesDefenses = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
            return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(QBIT_CLANKERS_LAB_DEFENSES_DESTROYED);
        });
        if (disablesDefenses && std::ranges::find(result, face.eventId) == result.end())
            result.push_back(face.eventId);
    }
    return result;
}

static std::pair<Vec3f, int> approachDarkGuildFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

static bool opensDarkGuildHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

static void leaveDarkGuildHouse(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    if (current_screen_type == SCREEN_HOUSE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void unlockDarkAdvisers(EngineController &game) {
    pParty->_questBits.set(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_ARCHIBALD_VISIT_ACTIVE);
    pParty->_questBits.set(QBIT_BREEDING_ZONE_ACTIVE);
    pParty->_questBits.set(QBIT_BREEDING_ZONE_FINISHED);
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    while (currentRestType == REST_WAIT)
        game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(-10300, -18432, -2861), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_DARK_GUILD_JOINED]);
    leaveDarkGuildHouse(game);
}

static void openPitAdviser(EngineController &game, int npcId) {
    const HouseId houseId = pNPCStats->pNPCData[npcId].house;
    ASSERT_NE(houseId, HOUSE_INVALID);
    game.teleportTo(MAP_PIT, Vec3f(), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [houseId](const BLVFace &face) {
        return face.Clickable() && opensDarkGuildHouse(face.eventId, houseId);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachDarkGuildFace(*door);
    game.teleportTo(MAP_PIT, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

GAME_TEST(MM7DarkGuild, BreedingZoneMarkerDoesNotGrantDarkGuild) {
    // The normal exit only returns the party to the Pit. The actual in-dungeon completion event must be found from
    // the loaded map program and clicked, so the test cannot silently substitute a generic exit for this objective.
    for (bool assigned : {false, true}) {
        SCOPED_TRACE(fmt::format("assigned={}", assigned));
        test.prepareForNextTest();
        auto activeTape = tapes.questBit(QBIT_BREEDING_ZONE_ACTIVE);
        auto finishedTape = tapes.questBit(QBIT_BREEDING_ZONE_FINISHED);
        auto guildTape = tapes.questBit(QBIT_DARK_GUILD_JOINED);
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        pParty->_questBits.set(QBIT_DARK_PATH);
        if (assigned)
            pParty->_questBits.set(QBIT_BREEDING_ZONE_ACTIVE);
        test.startTaping();

        game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
        const std::vector<int> completionEvents = breedingZoneCompletionEvents();
        ASSERT_EQ(completionEvents.size(), 1u);
        ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_BREEDING_ZONE, completionEvents.front(), 1, [] {
            return pParty->_questBits[QBIT_BREEDING_ZONE_FINISHED];
        }));

        // The dungeon's marker can be set before Archibald has assigned the task, but it is not the actual quest
        // completion: only the later Archibald hand-in may grant Dark-guild membership. Keep this distinction so a
        // future chain cannot mistake the map bit for an eligible adviser progression.
        EXPECT_EQ(activeTape, assigned ? tape(true) : tape(false));
        EXPECT_EQ(finishedTape, tape(false, true));
        EXPECT_EQ(guildTape, tape(false));
    }
}

GAME_TEST(MM7DarkGuild, ArchibaldAssignsBreedingZone) {
    // This is the post-Arbiter Dark route: reach Archibald through Castle Gloaming's actual night-only throne-room
    // door and accept his first dialogue topic. It deliberately does not fixture the Breeding Zone activation.
    auto activeTape = tapes.questBit(QBIT_BREEDING_ZONE_ACTIVE);
    auto houseTape = tapes.house();
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_ARCHIBALD_VISIT_ACTIVE);
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    while (currentRestType == REST_WAIT)
        game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    test.startTaping();

    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(-10300, -18432, -2861), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_GLOAMING_THRONE_ROOM_1);
    ASSERT_EQ(houseNpcs.size(), 1u);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();

    EXPECT_EQ(activeTape, tape(false, true));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_CASTLE_GLOAMING_THRONE_ROOM_1));
}

GAME_TEST(MM7DarkGuild, ArchibaldCompletesBreedingZone) {
    // The map marker is not enough by itself: report it back through the same Dark throne-room topic to unlock the
    // guild and the three Dark advisers.
    auto activeTape = tapes.questBit(QBIT_BREEDING_ZONE_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_BREEDING_ZONE_FINISHED);
    auto guildTape = tapes.questBit(QBIT_DARK_GUILD_JOINED);
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_ARCHIBALD_VISIT_ACTIVE);
    pParty->_questBits.set(QBIT_BREEDING_ZONE_ACTIVE);
    pParty->_questBits.set(QBIT_BREEDING_ZONE_FINISHED);
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    while (currentRestType == REST_WAIT)
        game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    test.startTaping();

    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(-10300, -18432, -2861), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_GLOAMING_THRONE_ROOM_1);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();

    EXPECT_EQ(activeTape, tape(true, false));
    EXPECT_EQ(finishedTape, tape(true));
    EXPECT_EQ(guildTape, tape(false, true));
}

GAME_TEST(MM7DarkGuild, KastoreRequiresBothAltarPieces) {
    // Kastore's Dark-side altar task has the same two physical pieces as the Light quest, but a distinct recipient
    // and completion bit. Reach him through the genuine Archibald hand-in and his runtime-selected Pit house.
    auto activeTape = tapes.questBit(QBIT_ALTAR_PIECES_DARK_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_ALTAR_PIECES_DARK_FINISHED);
    auto lightPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT);
    auto darkPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_DARK);
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    unlockDarkAdvisers(game);
    test.startTaping();

    openPitAdviser(game, 84);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_ACTIVE]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 84);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_FINISHED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 84);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_DARK));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_DARK));
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(lightPieceTape, tape(false, true, false));
    EXPECT_EQ(darkPieceTape, tape(false, true, false));
}

GAME_TEST(MM7DarkGuild, MaximusRequiresAndAcceptsSoulJars) {
    // Complete Kastore's prerequisite through his real Pit placement, then verify Maximus both assigns the distinct
    // Dark task and consumes the case. The Thunderfist acquisition event is covered separately because it is an
    // interaction-heavy dungeon source rather than a dialogue condition.
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    unlockDarkAdvisers(game);
    openPitAdviser(game, 84);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_ACTIVE]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 84);
    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_DARK));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_FINISHED]);
    leaveDarkGuildHouse(game);

    auto activeTape = tapes.questBit(QBIT_SOUL_JARS_DARK_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_SOUL_JARS_DARK_FINISHED);
    auto caseTape = tapes.hasItem(ITEM_QUEST_CASE_OF_SOUL_JARS);
    test.startTaping();
    openPitAdviser(game, 85);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_DARK_ACTIVE]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 85);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_SOUL_JARS_DARK_FINISHED]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 85);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CASE_OF_SOUL_JARS));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(caseTape, tape(false, true, false));
}

GAME_TEST(MM7DarkGuild, ClankerDefensesAreDisabledByMapEvent) {
    // Dark Shade's objective is the lab's magical and teleport defenses, not a kill-all sweep. Locate the live
    // event that records those defenses as destroyed and exercise it without actors present.
    auto activeTape = tapes.questBit(QBIT_CLANKERS_LAB_ACTIVE);
    auto destroyedTape = tapes.questBit(QBIT_CLANKERS_LAB_DEFENSES_DESTROYED);
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_DARK_GUILD_JOINED);
    pParty->_questBits.set(QBIT_CLANKERS_LAB_ACTIVE);
    test.startTaping();

    game.teleportTo(MAP_CLANKERS_LABORATORY, Vec3f(), 0);
    const std::vector<int> defenseEvents = clankerDefenseEvents();
    ASSERT_EQ(defenseEvents.size(), 1u);
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_CLANKERS_LABORATORY, defenseEvents.front(), 1, [] {
        return pParty->_questBits[QBIT_CLANKERS_LAB_DEFENSES_DESTROYED];
    }));

    EXPECT_EQ(activeTape, tape(true));
    EXPECT_EQ(destroyedTape, tape(false, true));
}

GAME_TEST(MM7DarkGuild, DarkShadeRequiresDisabledClankerDefenses) {
    // The dialogue contract is deliberately paired with the map-event contract above: Dark Shade must not accept a
    // generic dungeon visit or a kill-all substitute; only the defense marker completes this adviser task.
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    unlockDarkAdvisers(game);
    auto activeTape = tapes.questBit(QBIT_CLANKERS_LAB_ACTIVE);
    auto defensesTape = tapes.questBit(QBIT_CLANKERS_LAB_DEFENSES_DESTROYED);
    auto finishedTape = tapes.questBit(QBIT_CLANKERS_LAB_FINISHED);
    test.startTaping();

    openPitAdviser(game, 86);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_CLANKERS_LAB_ACTIVE]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 86);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_CLANKERS_LAB_FINISHED]);
    leaveDarkGuildHouse(game);

    pParty->_questBits.set(QBIT_CLANKERS_LAB_DEFENSES_DESTROYED);
    openPitAdviser(game, 86);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(defensesTape, tape(false, true));
    EXPECT_EQ(finishedTape, tape(false, true));
}

GAME_TEST(MM7DarkGuild, TolbertiRequiresAndAcceptsRobertsCube) {
    // This is the Dark mirror of Robert's Light-side request: the Pit's Tolberti sends the party after Robert in
    // Celeste and accepts Robert's cube, not Tolberti's own cube or a Light-recipient hand-in.
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    unlockDarkAdvisers(game);
    pParty->_questBits.set(QBIT_ALTAR_PIECES_DARK_FINISHED);
    pParty->_questBits.set(QBIT_SOUL_JARS_DARK_FINISHED);
    pParty->_questBits.set(QBIT_CLANKERS_LAB_FINISHED);
    auto activeTape = tapes.questBit(QBIT_ROBERT_THE_WISE_ASSASSINATION_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_ROBERT_THE_WISE_ASSASSINATED);
    auto cubeTape = tapes.hasItem(ITEM_QUEST_CONTROL_CUBE);
    test.startTaping();

    openPitAdviser(game, 87);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ROBERT_THE_WISE_ASSASSINATION_ACTIVE]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 87);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_ROBERT_THE_WISE_ASSASSINATED]);
    leaveDarkGuildHouse(game);
    openPitAdviser(game, 87);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CONTROL_CUBE));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(cubeTape, tape(false, true, false));
}
