#pragma once

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "Core/Time/Time.h"
#include "Engine/AttackList.h"
#include "Engine/GameTitle.h"
#include "Engine/TurnEngine/TurnEngine.h"

namespace mm6 {

// The combat owners changed at the start of DoPrepareWorld and the scalar
// state of the same turn engine. The caller supplies the live owners, so this
// scope does not access MM7 globals.
struct FirstMapCombatTransientOwners {
    std::vector<AttackDescription> *attacks = nullptr;
    stru262_TurnBased *turnEngine = nullptr;
    std::vector<Pid> *nearActorTargets = nullptr;
    std::vector<unsigned int> *nearActorIds = nullptr;

    bool complete() const noexcept {
        return attacks && turnEngine && nearActorTargets && nearActorIds;
    }
};

// Mirrors DoPrepareWorld's pre-load reset in detached buffers first. Once all
// allocation and filtering succeed, only no-throw swaps publish the state.
// Later outdoor, engine, Party and resource effects are outside this scope.
class ScopedFirstMapCombatTransients {
 public:
    ScopedFirstMapCombatTransients(GameTitle title, FirstMapCombatTransientOwners owners,
                                   int maxActors)
        : _owners(owners) {
        if (title != GameTitle::MM6 || !owners.complete() || maxActors < 0 ||
            maxActors > Pid::ID_MAX)
            throw std::runtime_error("MM6 first-map combat reset lacks validated owners");
        static_assert(std::is_nothrow_swappable_v<std::vector<AttackDescription>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<TurnBased_QueueElem>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<Pid>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<unsigned int>>);
        static_assert(std::is_nothrow_copy_assignable_v<Duration>);
        static_assert(std::is_nothrow_copy_assignable_v<TurnEngineFlags>);

        std::vector<TurnBased_QueueElem> nextQueue = owners.turnEngine->pQueue;
        std::erase_if(nextQueue, [](const TurnBased_QueueElem &entry) {
            return entry.uPackedID.type() == OBJECT_Actor;
        });
        std::vector<Pid> nextTargets = *owners.nearActorTargets;
        std::vector<unsigned int> nextIds = *owners.nearActorIds;
        nextTargets.resize(maxActors, Pid());
        nextIds.resize(maxActors);

        _priorTurnEngine = captureTurnEngine(*owners.turnEngine);
        _priorAttacks.swap(*owners.attacks);
        _priorQueue.swap(owners.turnEngine->pQueue);
        _priorTargets.swap(*owners.nearActorTargets);
        _priorIds.swap(*owners.nearActorIds);
        owners.turnEngine->pQueue.swap(nextQueue);
        owners.nearActorTargets->swap(nextTargets);
        owners.nearActorIds->swap(nextIds);
        _active = true;
    }

    ScopedFirstMapCombatTransients(const ScopedFirstMapCombatTransients &) = delete;
    ScopedFirstMapCombatTransients &operator=(const ScopedFirstMapCombatTransients &) = delete;
    ~ScopedFirstMapCombatTransients() { rollback(); }

    void rollback() noexcept {
        if (!_active)
            return;
        _priorIds.swap(*_owners.nearActorIds);
        _priorTargets.swap(*_owners.nearActorTargets);
        _priorQueue.swap(_owners.turnEngine->pQueue);
        _priorAttacks.swap(*_owners.attacks);
        restoreTurnEngine(*_owners.turnEngine, _priorTurnEngine);
        _active = false;
    }

    void commit() noexcept { _active = false; }

 private:
    struct TurnEngineScalars {
        int turnsCount = 0;
        TurnEngineStep stage = TE_NONE;
        Duration aiTimer;
        int initiative = 0;
        int actionPoints = 0;
        TurnEngineFlags flags;
        int pendingActions = 0;
    };

    static TurnEngineScalars captureTurnEngine(const stru262_TurnBased &engine) noexcept {
        return {engine.turns_count, engine.turn_stage, engine.ai_turn_timer,
                engine.turn_initiative, engine.uActionPointsLeft, engine.flags,
                engine.pending_actions};
    }

    static void restoreTurnEngine(stru262_TurnBased &engine,
                                  const TurnEngineScalars &state) noexcept {
        engine.turns_count = state.turnsCount;
        engine.turn_stage = state.stage;
        engine.ai_turn_timer = state.aiTimer;
        engine.turn_initiative = state.initiative;
        engine.uActionPointsLeft = state.actionPoints;
        engine.flags = state.flags;
        engine.pending_actions = state.pendingActions;
    }

    FirstMapCombatTransientOwners _owners;
    TurnEngineScalars _priorTurnEngine;
    std::vector<AttackDescription> _priorAttacks;
    std::vector<TurnBased_QueueElem> _priorQueue;
    std::vector<Pid> _priorTargets;
    std::vector<unsigned int> _priorIds;
    bool _active = false;
};

} // namespace mm6
