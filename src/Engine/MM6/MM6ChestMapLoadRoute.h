#pragma once

#include <utility>

#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/Tables/MapTable.h"

namespace mm6 {

// A scoped, thread-local integration gate for headless map-load acceptance.
// Production MM6 map loads keep the blocking guard, except the first map of a
// new game, see admitNewGameChestMapLoad. MM7 never enters this route, even if
// a scope exists on its thread.
namespace detail {

inline bool &chestMapLoadOptInFlag() {
    static thread_local bool enabled = false;
    return enabled;
}

inline bool &preserveMarkerFreeChestFlag() {
    static thread_local bool enabled = false;
    return enabled;
}

inline MapId &newGameChestMap() {
    static MapId map = MAP_INVALID;
    return map;
}

}  // namespace detail

inline bool chestMapLoadOptedIn() {
    return detail::chestMapLoadOptInFlag();
}

/**
 * Lets the next active chest load hand off the chest title state of a new game that `startNewGameSession` staged,
 * without a scoped opt-in. Only a load of `mapId` uses the admission, and the next active chest load of any map
 * consumes it.
 *
 * @param mapId                         First map of the new game.
 */
inline void admitNewGameChestMapLoad(MapId mapId) {
    detail::newGameChestMap() = mapId;
}

class ScopedChestMapLoadOptIn {
 public:
    explicit ScopedChestMapLoadOptIn(bool preserveMarkerFree = false)
        : previous_(detail::chestMapLoadOptInFlag()),
          previousPreserve_(detail::preserveMarkerFreeChestFlag()) {
        detail::chestMapLoadOptInFlag() = true;
        detail::preserveMarkerFreeChestFlag() = preserveMarkerFree;
    }

    ~ScopedChestMapLoadOptIn() {
        detail::chestMapLoadOptInFlag() = previous_;
        detail::preserveMarkerFreeChestFlag() = previousPreserve_;
    }

    ScopedChestMapLoadOptIn(const ScopedChestMapLoadOptIn &) = delete;
    ScopedChestMapLoadOptIn &operator=(const ScopedChestMapLoadOptIn &) = delete;

 private:
    bool previous_;
    bool previousPreserve_;
};

inline bool tryChestMapLoadBinding(GameTitle title, MapTreasureLevel mapLevel,
                                 const ItemTable *table, const Party *party,
                                 const ChestLiveBinding &binding) {
    if (title != GameTitle::MM6 || !chestMapLoadOptedIn() || !table || !party ||
        binding.title != title || !binding.titleState || !binding.titleState->ready)
        return false;
    const auto owned = scanPartyOwnedChestArtifacts(title, *party);
    return owned && applyChestLiveOwnerHandoff(binding, mapLevel, *table, *owned);
}

inline bool tryActiveChestMapLoad(GameTitle title, MapId mapId, const MapTable *maps,
                                 const ItemTable *table, const Party *party) {
    const bool newGameMap = std::exchange(detail::newGameChestMap(), MAP_INVALID) == mapId;
    if (title != GameTitle::MM6 || !(chestMapLoadOptedIn() || newGameMap) || !maps || !table || !party ||
        mapId < MAP_FIRST || mapId > MAP_LAST || !chestTitleState().ready)
        return false;
    const auto owned = scanPartyOwnedChestArtifacts(title, *party);
    return owned && applyActiveChestLiveOwnerHandoff(
        title, maps->pInfos[mapId].mapTreasureLevel, *table, *owned,
        detail::preserveMarkerFreeChestFlag());
}

}  // namespace mm6
