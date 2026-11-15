#include <algorithm>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/Party.h"

namespace {

constexpr int guardSetupEventId = 1;
constexpr int alliedGuardGroup = 4;
constexpr int wildernessGroup = 5;
constexpr int angerCounterIndex = 9;
constexpr EvtVariable guardStateVariable = static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentVariable_0) + 4);

bool eventChangesVariableAt(int step, EvtOpcode opcode, EvtVariable variable, int value) {
    return std::ranges::any_of(engine->_localEventMap.function(guardSetupEventId),
                               [=](const EvtInstruction &instruction) {
        return instruction.step == step && instruction.opcode == opcode &&
               instruction.data.variable_descr.type == variable &&
               instruction.data.variable_descr.value == value;
    });
}

bool eventTogglesGroupAt(int step, int group, bool set) {
    return std::ranges::any_of(engine->_localEventMap.function(guardSetupEventId),
                               [=](const EvtInstruction &instruction) {
        return instruction.step == step && instruction.opcode == EVENT_ToggleActorGroupFlag &&
               instruction.data.actor_flag_descr.id == group &&
               instruction.data.actor_flag_descr.attr == ACTOR_AGGRESSOR &&
               instruction.data.actor_flag_descr.is_set == set;
    });
}

void enterDeyja(EngineController &game) {
    game.teleportTo(MAP_DEYJA, Vec3f(), 0);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_DEYJA);
}

void expectAlliedGuardsFriendly() {
    int count = 0;
    for (Actor &actor : pActors) {
        if (actor.group != alliedGuardGroup)
            continue;
        ++count;
        EXPECT_EQ(monsterTypeForMonsterId(actor.monsterInfo.id), MONSTER_TYPE_GOBLIN);
        EXPECT_EQ(actor.hostilityGroup, MONSTER_TYPE_PEASANT_GOBLIN_FEMALE_A);
        EXPECT_FALSE(actor.attributes & ACTOR_AGGRESSOR);
        EXPECT_EQ(actor.GetActorsRelation(nullptr), HOSTILITY_FRIENDLY);
    }
    EXPECT_EQ(count, 8);
}

void expectAlliedGuardsHostile() {
    int count = 0;
    for (Actor &actor : pActors) {
        if (actor.group != alliedGuardGroup)
            continue;
        ++count;
        EXPECT_EQ(actor.hostilityGroup, monsterTypeForMonsterId(actor.monsterInfo.id));
        EXPECT_TRUE(actor.attributes & ACTOR_AGGRESSOR);
        EXPECT_NE(actor.GetActorsRelation(nullptr), HOSTILITY_FRIENDLY);
    }
    EXPECT_EQ(count, 8);
}

void expectWildernessHostile() {
    int count = 0;
    for (Actor &actor : pActors) {
        if (actor.group != wildernessGroup)
            continue;
        ++count;
        EXPECT_TRUE(actor.attributes & ACTOR_AGGRESSOR);
        EXPECT_NE(actor.GetActorsRelation(nullptr), HOSTILITY_FRIENDLY);
    }
    EXPECT_GT(count, 0);
}

} // namespace

GAME_TEST(MM7DeyjaGuards, SourceContract) {
    game.startNewGame();
    enterDeyja(game);
    const EvtProgram &program = engine->_localEventMap;
    ASSERT_TRUE(program.hasEvent(guardSetupEventId));
    EXPECT_EQ(program.instruction(guardSetupEventId, 0).opcode, EVENT_OnMapReload);
    EXPECT_TRUE(eventChangesVariableAt(2, EVENT_Compare, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_LIGHT_PATH)));
    EXPECT_EQ(program.instruction(guardSetupEventId, 2).target_step, 6);
    EXPECT_TRUE(eventChangesVariableAt(3, EVENT_Compare, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_ALLIED_GUARDS_ANGERED)));
    EXPECT_EQ(program.instruction(guardSetupEventId, 3).target_step, 10);
    EXPECT_TRUE(eventChangesVariableAt(4, EVENT_Compare, guardStateVariable, 2));
    EXPECT_EQ(program.instruction(guardSetupEventId, 4).target_step, 7);
    EXPECT_TRUE(eventTogglesGroupAt(7, alliedGuardGroup, true));
    EXPECT_TRUE(eventTogglesGroupAt(8, wildernessGroup, true));
    EXPECT_TRUE(eventChangesVariableAt(10, EVENT_Compare, VAR_Counter10, 720));
    EXPECT_EQ(program.instruction(guardSetupEventId, 10).target_step, 12);
    EXPECT_TRUE(eventChangesVariableAt(12, EVENT_Subtract, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_ALLIED_GUARDS_ANGERED)));
    EXPECT_TRUE(eventChangesVariableAt(13, EVENT_Set, guardStateVariable, 0));
    EXPECT_TRUE(eventTogglesGroupAt(14, alliedGuardGroup, false));
    EXPECT_FALSE(std::ranges::any_of(program.function(guardSetupEventId), [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_ChangeGroupAlly;
    }));
}

GAME_TEST(MM7DeyjaGuards, NeutralAndDarkPathsKeepNorthernGuardsFriendly) {
    for (bool dark : {false, true}) {
        SCOPED_TRACE(fmt::format("dark={}", dark));
        test.prepareForNextTest();
        game.startNewGame();
        if (dark)
            pParty->_questBits.set(QBIT_DARK_PATH);

        enterDeyja(game);

        expectAlliedGuardsFriendly();
        expectWildernessHostile();
    }
}

GAME_TEST(MM7DeyjaGuards, LightPathUsesDefaultHostileFaction) {
    game.startNewGame();
    pParty->_questBits.set(QBIT_LIGHT_PATH);

    enterDeyja(game);

    EXPECT_EQ(engine->_persistentVariables.mapVars[4], 2);
    expectAlliedGuardsHostile();
    expectWildernessHostile();
}

GAME_TEST(MM7DeyjaGuards, AngerExpiresAtThirtyDays) {
    game.startNewGame();
    pParty->_questBits.set(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_ALLIED_GUARDS_ANGERED);
    const Time angerStartedAt = pParty->GetPlayingTime();
    pParty->PartyTimes.CounterEventValues[angerCounterIndex] = angerStartedAt;
    pParty->GetPlayingTime() = angerStartedAt + Duration::fromHours(719);

    enterDeyja(game);

    EXPECT_TRUE(pParty->_questBits[QBIT_ALLIED_GUARDS_ANGERED]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[4], 2);
    expectAlliedGuardsHostile();
    expectWildernessHostile();

    pParty->GetPlayingTime() = angerStartedAt + Duration::fromHours(720);
    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    enterDeyja(game);

    EXPECT_FALSE(pParty->_questBits[QBIT_ALLIED_GUARDS_ANGERED]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[4], 0);
    expectAlliedGuardsFriendly();
    expectWildernessHostile();
}
