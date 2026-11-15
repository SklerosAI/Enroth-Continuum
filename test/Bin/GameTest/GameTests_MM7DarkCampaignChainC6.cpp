#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

static bool isXenofex(const Actor &actor) {
    return actor.uniqueNameIndex != 0 && pMonsterStats->uniqueNames[actor.uniqueNameIndex] == "Xenofex";
}

static DoorState colonyDoorState(int doorId) {
    auto door = std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId);
    return door == pIndoor->doors.end() ? DOOR_CLOSED : door->state;
}

static void openGloamingKastore(EngineController &game) {
    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(-10300, -18432, -2861), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_GLOAMING_THRONE_ROOM_2);
    auto kastore = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[84];
    });
    ASSERT_NE(kastore, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", kastore - houseNpcs.begin()));
        game.tick();
    }
}

static void useOutdoorTransition(EngineController &game, MapId map, int eventId) {
    auto finishPendingMove = [&] {
        for (int frame = 0; frame < 5 && current_screen_type != SCREEN_INPUT_BLV && engine->_currentLoadedMapId == map; ++frame)
            game.tick();
    };
    auto entrance = std::ranges::find_if(pOutdoor->faces, [eventId](const BLVFace &face) {
        return face.eventId == eventId && face.Clickable();
    });
    if (entrance != pOutdoor->faces.end()) {
        auto [position, yaw] = approachCampaignChainFace(*entrance);
        game.teleportTo(map, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        finishPendingMove();
    }
    for (int faceId = 0; faceId < pOutdoor->faces.size() && current_screen_type != SCREEN_INPUT_BLV && engine->_currentLoadedMapId == map;
         ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.eventId != eventId || !face.Clickable())
            continue;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                 std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
            game.teleportTo(map, face.boundingBox.center() + offset, yaw, -30);
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            finishPendingMove();
            if (current_screen_type == SCREEN_INPUT_BLV || engine->_currentLoadedMapId != map)
                break;
        }
    }
    auto trigger = std::ranges::find(pOutdoor->faces, eventId, &BLVFace::eventId);
    if (current_screen_type != SCREEN_INPUT_BLV && engine->_currentLoadedMapId == map && trigger != pOutdoor->faces.end() && trigger->Pressure_Plate()) {
        Vec3f position = trigger->boundingBox.center();
        position.z = trigger->boundingBox.z1 + 50;
        game.teleportTo(map, position, 0);
        game.tick(5);
        finishPendingMove();
    }
    auto decoration = std::ranges::find(pLevelDecorations, eventId, &LevelDecoration::uEventID);
    if (current_screen_type != SCREEN_INPUT_BLV && engine->_currentLoadedMapId == map && decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(map, decoration->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            finishPendingMove();
            if (current_screen_type == SCREEN_INPUT_BLV || engine->_currentLoadedMapId != map)
                break;
        }
    }
    if (current_screen_type == SCREEN_INPUT_BLV) {
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
    } else {
        ASSERT_NE(engine->_currentLoadedMapId, map);
        game.tick(2); // The direct altar transition has two post-load input-skip frames.
    }
}

static bool openIndoorFaceEvent(EngineController &game, MapId map, int eventId) {
    if (clickCampaignChainEventFace(game, map, eventId, 1, [] { return current_screen_type == SCREEN_INPUT_BLV; }))
        return true;
    auto safeFloor = [](Vec3f position) {
        int sector = pIndoor->GetSector(position);
        int floorFace = -1;
        return sector != 0 && BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius), sector, &floorFace) != -30000 && floorFace != -1;
    };
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        const int faceId = &face - pIndoor->faces.data();
        if (std::abs(face.facePlane.normal.z) > 0.5f) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                     std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
                Vec3f position = face.boundingBox.center() + offset;
                if (!safeFloor(position))
                    continue;
                game.teleportTo(map, position, yaw, -45);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (current_screen_type == SCREEN_INPUT_BLV)
                    return true;
            }
        }
        for (float distance : {150.0f, 300.0f, -150.0f, -300.0f}) {
            for (float height : {face.boundingBox.z1, face.boundingBox.center().z}) {
                Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
                position.z = height;
                if (!safeFloor(position))
                    continue;
                int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
                if (distance < 0)
                    yaw += 180;
                game.teleportTo(map, position, yaw);
                game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                game.tick();
                if (current_screen_type == SCREEN_INPUT_BLV)
                    return true;
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (current_screen_type == SCREEN_INPUT_BLV)
                    return true;
            }
        }
    }
    return false;
}

static void useIndoorTransition(EngineController &game, MapId map, int eventId) {
    auto face = std::ranges::find_if(pIndoor->faces, [eventId](const BLVFace &candidate) {
        return candidate.eventId == eventId && candidate.Clickable();
    });
    if (face != pIndoor->faces.end()) {
        ASSERT_TRUE(openIndoorFaceEvent(game, map, eventId));
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        return;
    }

    face = std::ranges::find(pIndoor->faces, eventId, &BLVFace::eventId);
    if (face != pIndoor->faces.end()) {
        ASSERT_TRUE(face->Pressure_Plate());
        Vec3f position = face->boundingBox.center();
        position.z = face->boundingBox.z1 + 50;
        game.teleportTo(map, position, 0);
        for (int frame = 0; frame < 10 && current_screen_type != SCREEN_INPUT_BLV; ++frame)
            game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        return;
    }

    auto decoration = std::ranges::find(pLevelDecorations, eventId, &LevelDecoration::uEventID);
    ASSERT_NE(decoration, pLevelDecorations.end());
    const int decorationId = decoration - pLevelDecorations.begin();
    bool pointed = false;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
             std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
        game.teleportTo(map, decoration->vPosition + offset, yaw);
        try {
            game.pointMouseAtDecoration(decorationId);
            pointed = true;
            break;
        } catch (const Exception &) {
        }
    }
    ASSERT_TRUE(pointed);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
}

static void rescueRolandAndOpenRoute(EngineController &game) {
    auto cage = std::ranges::find(pLevelDecorations, 376, &LevelDecoration::uEventID);
    ASSERT_NE(cage, pLevelDecorations.end());
    game.teleportTo(MAP_COLONY_ZOD, Vec3f(-10986, 8576, 1728), 180); // On the ledge east of the hanging cage.
    game.pointMouseAtDecoration(cage - pLevelDecorations.begin());
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    ASSERT_TRUE(pParty->_questBits[QBIT_TALKED_TO_ROLAND]);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_COLONY_ZOD_KEY);

    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_COLONY_ZOD, 377, 1, [] {
        return colonyDoorState(32) != DOOR_CLOSED;
    }));
    for (int frame = 0; frame < 100 && colonyDoorState(32) != DOOR_OPEN; ++frame)
        game.tick();
    ASSERT_EQ(colonyDoorState(32), DOOR_OPEN);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);

    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_COLONY_ZOD, 23, 1, [] {
        return engine->_persistentVariables.mapVars[2] == 1;
    }));
    ASSERT_EQ(engine->_persistentVariables.mapVars[1], 1);
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_COLONY_ZOD, 24, 1, [] {
        return engine->_persistentVariables.mapVars[3] == 1;
    }));
    ASSERT_EQ(engine->_persistentVariables.mapVars[1], 2);
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_COLONY_ZOD, 18, 1, [] {
        return colonyDoorState(30) != DOOR_CLOSED;
    }));
    for (int frame = 0; frame < 100 && colonyDoorState(30) != DOOR_OPEN; ++frame)
        game.tick();
    ASSERT_EQ(colonyDoorState(30), DOOR_OPEN);
}

static int fightXenofex(EngineController &game) {
    std::vector<int> candidates;
    for (const Actor &actor : pActors)
        if (isXenofex(actor))
            candidates.push_back(actor.id);
    EXPECT_EQ(candidates.size(), 1u);
    if (candidates.size() != 1)
        return -1;
    const int xenofexId = candidates.front();
    EXPECT_EQ(pActors[xenofexId].monsterInfo.id, MONSTER_DEVIL_C);
    EXPECT_FALSE(pActors[xenofexId].IsNotAlive());
    auto follow = [&] {
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(0, -250, 0), 90}, std::pair{Vec3f(-250, 0, 0), 0},
                 std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(250, 0, 0), 180}}) {
            for (int pitch : {-20, 0, 20}) {
                game.teleportTo(MAP_COLONY_ZOD, pActors[xenofexId].pos + offset, yaw, pitch);
                try {
                    game.pointMouseAtActor(xenofexId);
                    return true;
                } catch (const Exception &) {
                }
            }
        }
        return false;
    };
    EXPECT_TRUE(follow());
    for (int attack = 0; attack < 1000 && !pActors[xenofexId].IsNotAlive(); ++attack) {
        if (attack && attack % 20 == 0)
            EXPECT_TRUE(follow());
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    EXPECT_TRUE(pActors[xenofexId].IsNotAlive()) << "remaining HP: " << pActors[xenofexId].hp;
    return xenofexId;
}

template<class T>
static std::optional<T> onColonyZod(T value) {
    return engine->_currentLoadedMapId == MAP_COLONY_ZOD ? std::optional(value) : std::nullopt;
}

GAME_TEST(MM7DarkCampaignChain, DISABLED_Chapter6) {
    // Fixture: the party teleports to Mount Nighon before following the authored Thunderfist/Tunnels/Land/Zod
    // access chain. Teleports approach the cage, controls and live Xenofex; every map boundary, key/control event,
    // kill, return altar and Kastore dialogue uses the production interaction path.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save006.mm7"));
    loadCampaignChainSlot(game, 5);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_PIT);
    ASSERT_TRUE(pParty->_questBits[QBIT_DARK_PATH]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ROBERT_THE_WISE_ASSASSINATED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_XENOFEX_DARK_ACTIVE]);
    for (Character &character : pParty->pCharacters)
        character.health = character.GetMaxHealth();

    auto activeTape = tapes.questBit(QBIT_XENOFEX_DARK_ACTIVE);
    auto rolandTape = tapes.questBit(QBIT_TALKED_TO_ROLAND);
    auto keyTape = tapes.totalItemCount(ITEM_COLONY_ZOD_KEY);
    auto killedTape = tapes.questBit(QBIT_XENOFEX_KILLED);
    auto finishedTape = tapes.questBit(QBIT_XENOFEX_QUEST_FINISHED);
    auto returnAltarTape = tapes.questBit(QBIT_LAND_OF_THE_GIANTS_ALTAR_USED);
    auto controlsTape = tapes.custom([] {
        return engine->_currentLoadedMapId == MAP_COLONY_ZOD
                   ? std::optional(std::array{engine->_persistentVariables.mapVars[1], engine->_persistentVariables.mapVars[2],
                                              engine->_persistentVariables.mapVars[3]})
                   : std::optional<std::array<unsigned char, 3>>();
    });
    auto xenofexAliveTape = tapes.custom([] {
        auto xenofex = std::ranges::find_if(pActors, isXenofex);
        return engine->_currentLoadedMapId == MAP_COLONY_ZOD && xenofex != pActors.end()
                   ? std::optional(!xenofex->IsNotAlive())
                   : std::nullopt;
    });
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();

    openGloamingKastore(game);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 84, 159));
    ASSERT_TRUE(pParty->_questBits[QBIT_XENOFEX_DARK_ACTIVE]);
    leaveCampaignChainHouse(game);

    engine->config->debug.NoActors.setValue(true);
    game.teleportTo(MAP_MOUNT_NIGHON, Vec3f(), 0);
    {
        SCOPED_TRACE("Mount Nighon to Thunderfist");
        useOutdoorTransition(game, MAP_MOUNT_NIGHON, 501);
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_THUNDERFIST_MOUNTAIN);
    {
        SCOPED_TRACE("Thunderfist to Tunnels to Eofol");
        useIndoorTransition(game, MAP_THUNDERFIST_MOUNTAIN, 506);
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TUNNELS_TO_EEOFOL);
    {
        SCOPED_TRACE("Tunnels to Eofol to Land of the Giants");
        useIndoorTransition(game, MAP_TUNNELS_TO_EEOFOL, 501);
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LAND_OF_THE_GIANTS);
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE); // Archibald greets a first arrival and gives the party a Blaster.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_BLASTER);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1); // Put the gift in the first character's pack before taking Roland's key.
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    engine->config->debug.NoActors.setValue(false);
    {
        SCOPED_TRACE("Land of the Giants to Colony Zod");
        useOutdoorTransition(game, MAP_LAND_OF_THE_GIANTS, 501);
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_COLONY_ZOD);

    rescueRolandAndOpenRoute(game);
    const int xenofexId = fightXenofex(game);
    ASSERT_NE(xenofexId, -1);
    ASSERT_TRUE(pActors[xenofexId].IsNotAlive());
    ASSERT_GT(pParty->canActCount(), 0);
    {
        SCOPED_TRACE("Colony Zod to Land of the Giants");
        useIndoorTransition(game, MAP_COLONY_ZOD, 501);
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LAND_OF_THE_GIANTS);
    ASSERT_TRUE(pParty->_questBits[QBIT_XENOFEX_KILLED]);
    {
        SCOPED_TRACE("Land of the Giants altar to Harmondale");
        useOutdoorTransition(game, MAP_LAND_OF_THE_GIANTS, 452);
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_LAND_OF_THE_GIANTS_ALTAR_USED]);

    engine->config->debug.NoActors.setValue(true);
    game.teleportTo(MAP_PIT, Vec3f(), 0); // Reload without actors before waiting for Castle Gloaming's night hours.
    game.tick(2);
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    while (currentRestType == REST_WAIT)
        game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    engine->config->debug.NoActors.setValue(false);
    openGloamingKastore(game);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 84, 160));
    ASSERT_TRUE(pParty->_questBits[QBIT_XENOFEX_QUEST_FINISHED]);
    leaveCampaignChainHouse(game);

    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(rolandTape, tape(false, true));
    EXPECT_EQ(keyTape, tape(0, 1, 0));
    EXPECT_EQ(killedTape, tape(false, true));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(returnAltarTape, tape(false, true));
    EXPECT_EQ(controlsTape, tape(std::optional<std::array<unsigned char, 3>>(),
                                 std::optional(std::array<unsigned char, 3>{0, 0, 0}),
                                 std::optional(std::array<unsigned char, 3>{1, 1, 0}),
                                 std::optional(std::array<unsigned char, 3>{2, 1, 1}),
                                 std::optional<std::array<unsigned char, 3>>()));
    EXPECT_EQ(xenofexAliveTape, tape(std::optional<bool>(), std::optional(true), std::optional(false), std::optional<bool>()));
    EXPECT_EQ(mapTape, tape(MAP_PIT, MAP_CASTLE_GLOAMING, MAP_MOUNT_NIGHON, MAP_THUNDERFIST_MOUNTAIN,
                            MAP_TUNNELS_TO_EEOFOL, MAP_LAND_OF_THE_GIANTS, MAP_COLONY_ZOD,
                            MAP_LAND_OF_THE_GIANTS, MAP_HARMONDALE, MAP_CASTLE_GLOAMING));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_CASTLE_GLOAMING_THRONE_ROOM_2, HOUSE_INVALID,
                              HOUSE_CASTLE_GLOAMING_THRONE_ROOM_2, HOUSE_INVALID));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 6);
}
