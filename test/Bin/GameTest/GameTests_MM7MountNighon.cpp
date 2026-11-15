#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/HouseTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int setupEventId = 1;
constexpr int warlockGroupA = 26;
constexpr int warlockGroupB = 27;

struct DoorHalf {
    int model;
    int face;
    int disabledEvent;
    int workingEvent;
    HouseId house;
};

constexpr std::array doorHalves = {
    DoorHalf{0, 38, 12, 11, HOUSE_TAVERN_MOUNT_NIGHON},
    DoorHalf{12, 17, 10, 9, HOUSE_TRAINING_HALL_MOUNT_NIGHON},
};

bool eventHasInstruction(int eventId, int step, EvtOpcode opcode) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step && instruction.opcode == opcode;
           });
}

bool eventOpensHouseAtStep(int eventId, int step, HouseId house) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step && instruction.opcode == EVENT_SpeakInHouse &&
                      instruction.data.house_id == house;
           });
}

void enterMountNighon(EngineController &game) {
    game.teleportTo(MAP_MOUNT_NIGHON, Vec3f(), 0);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_MOUNT_NIGHON);
}

void openDoorHalf(EngineController &game, const DoorHalf &door) {
    ASSERT_LT(door.model, std::ssize(pOutdoor->pBModels));
    const BSPModel &model = pOutdoor->pBModels[door.model];
    ASSERT_LT(door.face, std::ssize(model.faces));
    const int faceId = model.faces[door.face];
    const BLVFace &face = pOutdoor->faces[faceId];
    ASSERT_EQ(face.eventId, door.workingEvent);
    ASSERT_TRUE(face.Clickable());

    bool pointed = false;
    for (float distance : {100.0f, 175.0f, 250.0f, 400.0f}) {
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
            position.z = face.boundingBox.center().z - pParty->eyeLevel;
            const int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                   -direction * face.facePlane.normal.x) *
                                        180 / std::numbers::pi);
            game.teleportTo(MAP_MOUNT_NIGHON, position, yaw);
            try {
                game.pointMouseAtFace(faceId);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        if (pointed)
            break;
    }
    ASSERT_TRUE(pointed);
    const Vis_PIDAndDepth picked = engine->PickMouseForTargeting();
    ASSERT_EQ(picked.pid, Pid::face(faceId));
    ASSERT_LT(picked.depth, engine->config->gameplay.MouseInteractionDepth.value());
    ASSERT_TRUE(pParty->hasActiveCharacter());
    ASSERT_EQ(pOutdoor->faces[faceId].eventId, door.workingEvent);
    engine->onGameViewportClick();
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    EXPECT_EQ(window_SpeakInHouse->houseId(), door.house);
}

} // namespace

GAME_TEST(MM7MountNighon, SourceContracts) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    enterMountNighon(game);

    ASSERT_TRUE(engine->_localEventMap.hasEvent(setupEventId));
    const std::span<const EvtInstruction> setup = engine->_localEventMap.function(setupEventId);
    EXPECT_EQ(setup.size(), 7);
    EXPECT_TRUE(eventHasInstruction(setupEventId, 0, EVENT_OnMapReload));
    EXPECT_TRUE(std::ranges::any_of(setup, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_TOWN_PORTAL_TO_MOUNT_NIGHON);
    }));
    EXPECT_TRUE(std::ranges::any_of(setup, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_TOWN_PORTAL_TO_MOUNT_NIGHON);
    }));
    EXPECT_TRUE(std::ranges::any_of(setup, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.opcode == EVENT_ToggleActorGroupFlag &&
               instruction.data.actor_flag_descr.id == 5 &&
               instruction.data.actor_flag_descr.attr == ACTOR_AGGRESSOR &&
               instruction.data.actor_flag_descr.is_set;
    }));
    EXPECT_FALSE(std::ranges::any_of(setup, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_ChangeGroupAlly;
    }));

    for (const DoorHalf &door : doorHalves) {
        SCOPED_TRACE(fmt::format("model={}, face={}", door.model, door.face));
        EXPECT_TRUE(eventHasInstruction(door.disabledEvent, 0, EVENT_Exit));
        EXPECT_TRUE(eventOpensHouseAtStep(door.disabledEvent, 1, door.house));
        EXPECT_TRUE(eventOpensHouseAtStep(door.workingEvent, 0, door.house));
        EXPECT_EQ(engine->_localEventMap.function(door.disabledEvent).size(), 3);
        EXPECT_EQ(engine->_localEventMap.function(door.workingEvent).size(), 2);
        ASSERT_LT(door.model, std::ssize(pOutdoor->pBModels));
        ASSERT_LT(door.face, std::ssize(pOutdoor->pBModels[door.model].faces));
        const int faceId = pOutdoor->pBModels[door.model].faces[door.face];
        EXPECT_EQ(pOutdoor->faces[faceId].faceId, faceId);
        EXPECT_TRUE(pOutdoor->faces[faceId].Clickable());
    }
}

GAME_TEST(MM7MountNighon, TavernDoorHalfOpensTheTavern) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    enterMountNighon(game);

    openDoorHalf(game, doorHalves[0]);
}

GAME_TEST(MM7MountNighon, TrainingDoorHalfOpensTheTrainingHall) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->GetPlayingTime() = Time::fromDays(pParty->GetPlayingTime().toDays()) +
                               Duration::fromHours(houseTable[HOUSE_TRAINING_HALL_MOUNT_NIGHON].uOpenTime + 1);
    enterMountNighon(game);

    openDoorHalf(game, doorHalves[1]);
}

GAME_TEST(MM7MountNighon, WarlockGroupsRemainFriendlyToTheWarlocks) {
    game.startNewGame();
    enterMountNighon(game);

    Actor *warlock = nullptr;
    std::array<int, 2> correctedCounts = {};
    for (Actor &actor : pActors) {
        if (!warlock && monsterTypeForMonsterId(actor.monsterInfo.id) == MONSTER_TYPE_WARLOCK &&
            actor.group != warlockGroupA && actor.group != warlockGroupB)
            warlock = &actor;
        if (actor.group == warlockGroupA)
            ++correctedCounts[0];
        if (actor.group == warlockGroupB)
            ++correctedCounts[1];
    }
    ASSERT_NE(warlock, nullptr);
    EXPECT_GT(correctedCounts[0], 0);
    EXPECT_GT(correctedCounts[1], 0);

    for (Actor &actor : pActors) {
        if (actor.group != warlockGroupA && actor.group != warlockGroupB)
            continue;
        EXPECT_EQ(actor.hostilityGroup, MONSTER_TYPE_WARLOCK);
        EXPECT_EQ(actor.GetActorsRelation(warlock), HOSTILITY_FRIENDLY);
        EXPECT_EQ(warlock->GetActorsRelation(&actor), HOSTILITY_FRIENDLY);
    }
}
