#include "MM7CombatHarness.h"

#include <fmt/format.h>

#include <algorithm>
#include <string>

#include "Engine/Components/Control/EngineController.h"
#include "Engine/Engine.h"
#include "Engine/Objects/Actor.h"
#include "GUI/GUIWindow.h"

#include "Utility/Exception.h"

namespace mm7::testing {

CombatResult fightActorWithAttackInput(EngineController &game, int actorId,
                                       const TargetReacquirer &reacquireTarget,
                                       int maxAttacks, int reacquireEvery,
                                       int ticksPerAttack, int maxDeathSettleFrames) {
    if (actorId < 0 || actorId >= static_cast<int>(pActors.size()))
        throw Exception("MM7 combat target {} is outside the actor list", actorId);
    if (!reacquireTarget)
        throw Exception("MM7 combat target {} has no targeting callback", actorId);
    if (maxAttacks <= 0 || reacquireEvery <= 0 || ticksPerAttack <= 0 || maxDeathSettleFrames < 0)
        throw Exception("Invalid MM7 combat bounds: attacks={}, reacquire={}, ticks={}, settle={}",
                        maxAttacks, reacquireEvery, ticksPerAttack, maxDeathSettleFrames);

    Actor &target = pActors[actorId];
    CombatResult result;
    result.actorId = actorId;
    result.monsterId = std::to_underlying(target.monsterInfo.id);
    result.uniqueNameIndex = target.uniqueNameIndex;
    result.actorGroup = target.group;
    result.initialHp = target.hp;
    result.lowestHp = target.hp;
    result.initialState = target.aiState;
    result.gameTitle = engine->gameTitle();
    result.initialMap = engine->_currentLoadedMapId;
    result.initialScreen = current_screen_type;

    for (int attack = 0; attack < maxAttacks && !target.IsNotAlive(); ++attack) {
        if (attack % reacquireEvery == 0) {
            if (!reacquireTarget()) {
                result.targetingFailed = true;
                break;
            }
            result.targetAcquired = true;
        }

        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(ticksPerAttack);
        ++result.attacks;
        result.lowestHp = std::min(result.lowestHp, static_cast<int>(target.hp));
        result.observedHpDecrease |= target.hp < result.initialHp;
    }

    result.reachedTerminalDeath = target.IsNotAlive();
    for (int frame = 0; frame < maxDeathSettleFrames && target.aiState != Dead; ++frame) {
        game.tick();
        ++result.deathSettleFrames;
    }

    result.finalHp = target.hp;
    result.finalState = target.aiState;
    result.reachedDeadState = target.aiState == Dead;
    result.finalMap = engine->_currentLoadedMapId;
    result.finalScreen = current_screen_type;
    return result;
}

std::string describeCombatResult(const CombatResult &result) {
    return fmt::format(
        "title={}, map={}->{}, screen={}->{}, actor={}, monster={}, unique={}, group={}, attacks={}, "
        "hp={}->{} (low={}), state={}->{}, acquired={}, target_failure={}, damage={}, terminal={}, "
        "dead={}, settle_frames={}",
        std::to_underlying(result.gameTitle), std::to_underlying(result.initialMap),
        std::to_underlying(result.finalMap), std::to_underlying(result.initialScreen),
        std::to_underlying(result.finalScreen), result.actorId, result.monsterId,
        result.uniqueNameIndex, result.actorGroup, result.attacks, result.initialHp, result.finalHp, result.lowestHp,
        std::to_underlying(result.initialState), std::to_underlying(result.finalState),
        result.targetAcquired, result.targetingFailed, result.observedHpDecrease,
        result.reachedTerminalDeath, result.reachedDeadState, result.deathSettleFrames);
}

} // namespace mm7::testing
