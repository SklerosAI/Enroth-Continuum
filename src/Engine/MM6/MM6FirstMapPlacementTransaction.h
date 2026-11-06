#pragma once

#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Engine/EngineEnums.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/GameTitle.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"

namespace mm6 {

// Only the first-map Party placement and local transition/event owners.
// Persistent event variables, world objects and UI/resource effects are not
// included, so this is a test-only scope rather than a live load transaction.
struct FirstMapPlacementOwners {
    Party *party = nullptr;
    MapId *currentMap = nullptr;
    std::optional<MapDestination> *pendingTransition = nullptr;
    EvtProgram *localEvents = nullptr;
    std::vector<std::string> *levelStrings = nullptr;
    std::array<unsigned char, 50> *transientVariables = nullptr;
    std::string *lastLoadedSave = nullptr;
    LevelType *levelType = nullptr;
    EngineFlags *flags = nullptr;
};

class ScopedFirstMapPlacementAndEvents {
 public:
    ScopedFirstMapPlacementAndEvents(GameTitle title, FirstMapPlacementOwners owners,
                                     const Party &stagedParty, MapDestination destination)
        : _owners(owners) {
        if (title != GameTitle::MM6 || !owners.party || !owners.currentMap ||
            !owners.pendingTransition || !owners.localEvents || !owners.levelStrings ||
            !owners.transientVariables || !owners.lastLoadedSave || !owners.levelType ||
            !owners.flags || destination.map() == MAP_INVALID ||
            stagedParty.pos.x != -10178 || stagedParty.pos.y != -11206 ||
            stagedParty.pos.z != 340 || stagedParty.uFallStartZ != 340)
            throw std::runtime_error("MM6 first-map placement lacks validated owners");
        static_assert(std::is_nothrow_swappable_v<EvtProgram>);
        static_assert(std::is_nothrow_swappable_v<std::vector<std::string>>);
        static_assert(std::is_nothrow_swappable_v<std::optional<MapDestination>>);
        static_assert(std::is_nothrow_swappable_v<std::string>);
        static_assert(std::is_nothrow_copy_assignable_v<PartyPlacementState>);

        // Build the new destination before touching any caller owner.
        std::optional<MapDestination> next(std::move(destination));
        _priorParty = capturePlacement(*owners.party);
        _priorMap = *owners.currentMap;
        _priorLevelType = *owners.levelType;
        _priorFlags = *owners.flags;
        _priorTransient = *owners.transientVariables;
        std::swap(_priorEvents, *owners.localEvents);
        _priorStrings.swap(*owners.levelStrings);
        _priorLastSave.swap(*owners.lastLoadedSave);
        _priorPending.swap(*owners.pendingTransition);
        owners.pendingTransition->swap(next);
        applyPlacement(*owners.party, capturePlacement(stagedParty));
        *owners.currentMap = owners.pendingTransition->value().map();
        *owners.levelType = LEVEL_NULL;
        _active = true;
    }

    ScopedFirstMapPlacementAndEvents(const ScopedFirstMapPlacementAndEvents &) = delete;
    ScopedFirstMapPlacementAndEvents &operator=(const ScopedFirstMapPlacementAndEvents &) = delete;
    ~ScopedFirstMapPlacementAndEvents() { rollback(); }

    void rollback() noexcept {
        if (!_active)
            return;
        applyPlacement(*_owners.party, _priorParty);
        *_owners.currentMap = _priorMap;
        *_owners.levelType = _priorLevelType;
        *_owners.flags = _priorFlags;
        *_owners.transientVariables = _priorTransient;
        _priorPending.swap(*_owners.pendingTransition);
        _priorLastSave.swap(*_owners.lastLoadedSave);
        _priorStrings.swap(*_owners.levelStrings);
        std::swap(_priorEvents, *_owners.localEvents);
        _active = false;
    }

    void commit() noexcept { _active = false; }

 private:
    struct PartyPlacementState {
        Vec3f pos;
        Vec3f velocity;
        int yaw = 0;
        int pitch = 0;
        int fallStartZ = 0;
        int savedFlightZ = 0;
        int floorFace = -1;
        SoundId walkingSound = SOUND_Invalid;
        Duration delayedReaction;
        Time waterLavaTimer;
        Duration armageddonTimer;
        unsigned flying = 0;
        PartyFlags flags;
        PARTY_FLAGS_2 flags2 = {};
    };

    static PartyPlacementState capturePlacement(const Party &party) noexcept {
        return {party.pos, party.velocity, party._viewYaw, party._viewPitch,
                party.uFallStartZ, party.sPartySavedFlightZ, party.floor_face_id,
                party.currentWalkingSound, party._delayedReactionTimer,
                party._6FC_water_lava_timer, party.armageddon_timer,
                party.bFlying, party.uFlags, party.uFlags2};
    }

    static void applyPlacement(Party &party, const PartyPlacementState &state) noexcept {
        party.pos = state.pos;
        party.velocity = state.velocity;
        party._viewYaw = state.yaw;
        party._viewPitch = state.pitch;
        party.uFallStartZ = state.fallStartZ;
        party.sPartySavedFlightZ = state.savedFlightZ;
        party.floor_face_id = state.floorFace;
        party.currentWalkingSound = state.walkingSound;
        party._delayedReactionTimer = state.delayedReaction;
        party._6FC_water_lava_timer = state.waterLavaTimer;
        party.armageddon_timer = state.armageddonTimer;
        party.bFlying = state.flying;
        party.uFlags = state.flags;
        party.uFlags2 = state.flags2;
    }

    FirstMapPlacementOwners _owners;
    PartyPlacementState _priorParty;
    MapId _priorMap = MAP_INVALID;
    LevelType _priorLevelType = LEVEL_NULL;
    EngineFlags _priorFlags = {};
    std::array<unsigned char, 50> _priorTransient = {};
    std::optional<MapDestination> _priorPending;
    EvtProgram _priorEvents;
    std::vector<std::string> _priorStrings;
    std::string _priorLastSave;
    bool _active = false;
};

} // namespace mm6
