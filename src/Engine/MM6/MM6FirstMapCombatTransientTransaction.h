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

// The four combat owners changed at the start of DoPrepareWorld. The caller
// supplies the live containers, so this scope does not access MM7 globals.
struct FirstMapCombatTransientOwners {
    std::vector<AttackDescription> *attacks = nullptr;
    std::vector<TurnBased_QueueElem> *turnQueue = nullptr;
    std::vector<Pid> *nearActorTargets = nullptr;
    std::vector<unsigned int> *nearActorIds = nullptr;

    bool complete() const noexcept {
        return attacks && turnQueue && nearActorTargets && nearActorIds;
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

        std::vector<TurnBased_QueueElem> nextQueue = *owners.turnQueue;
        std::erase_if(nextQueue, [](const TurnBased_QueueElem &entry) {
            return entry.uPackedID.type() == OBJECT_Actor;
        });
        std::vector<Pid> nextTargets = *owners.nearActorTargets;
        std::vector<unsigned int> nextIds = *owners.nearActorIds;
        nextTargets.resize(maxActors, Pid());
        nextIds.resize(maxActors);

        _priorAttacks.swap(*owners.attacks);
        _priorQueue.swap(*owners.turnQueue);
        _priorTargets.swap(*owners.nearActorTargets);
        _priorIds.swap(*owners.nearActorIds);
        owners.turnQueue->swap(nextQueue);
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
        _priorQueue.swap(*_owners.turnQueue);
        _priorAttacks.swap(*_owners.attacks);
        _active = false;
    }

    void commit() noexcept { _active = false; }

 private:
    FirstMapCombatTransientOwners _owners;
    std::vector<AttackDescription> _priorAttacks;
    std::vector<TurnBased_QueueElem> _priorQueue;
    std::vector<Pid> _priorTargets;
    std::vector<unsigned int> _priorIds;
    bool _active = false;
};

} // namespace mm6
