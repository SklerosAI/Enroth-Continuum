#pragma once

#include <functional>
#include <string>

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ActorEnums.h"
#include "GUI/GUIEnums.h"

class EngineController;

namespace mm7::testing {

struct CombatResult {
    int actorId = -1;
    int monsterId = 0;
    int uniqueNameIndex = 0;
    int actorGroup = 0;
    int attacks = 0;
    int deathSettleFrames = 0;
    int initialHp = 0;
    int lowestHp = 0;
    int finalHp = 0;
    AIState initialState = Standing;
    AIState finalState = Standing;
    GameTitle gameTitle = GameTitle::MM7;
    MapId initialMap = MAP_INVALID;
    MapId finalMap = MAP_INVALID;
    ScreenType initialScreen = SCREEN_GAME;
    ScreenType finalScreen = SCREEN_GAME;
    bool targetAcquired = false;
    bool targetingFailed = false;
    bool observedHpDecrease = false;
    bool reachedTerminalDeath = false;
    bool reachedDeadState = false;
};

using TargetReacquirer = std::function<bool()>;

CombatResult fightActorWithAttackInput(EngineController &game, int actorId,
                                       const TargetReacquirer &reacquireTarget,
                                       int maxAttacks = 1000, int reacquireEvery = 10,
                                       int ticksPerAttack = 1, int maxDeathSettleFrames = 120);

std::string describeCombatResult(const CombatResult &result);

} // namespace mm7::testing
