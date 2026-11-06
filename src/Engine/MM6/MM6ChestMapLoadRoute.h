#pragma once

#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/Tables/MapTable.h"

namespace mm6 {

// A scoped, thread-local integration gate for headless map-load acceptance.
// Production MM6 remains on the existing blocking guard until a tested owner
// enables this scope around a complete map-load transaction. MM7 never enters
// this route, even if a scope exists on its thread.
namespace detail {

inline bool &chestMapLoadOptInFlag() {
    static thread_local bool enabled = false;
    return enabled;
}

}  // namespace detail

inline bool chestMapLoadOptedIn() {
    return detail::chestMapLoadOptInFlag();
}

class ScopedChestMapLoadOptIn {
 public:
    ScopedChestMapLoadOptIn() : previous_(detail::chestMapLoadOptInFlag()) {
        detail::chestMapLoadOptInFlag() = true;
    }

    ~ScopedChestMapLoadOptIn() {
        detail::chestMapLoadOptInFlag() = previous_;
    }

    ScopedChestMapLoadOptIn(const ScopedChestMapLoadOptIn &) = delete;
    ScopedChestMapLoadOptIn &operator=(const ScopedChestMapLoadOptIn &) = delete;

 private:
    bool previous_;
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
    if (title != GameTitle::MM6 || !chestMapLoadOptedIn() || !maps || !table || !party ||
        mapId < MAP_FIRST || mapId > MAP_LAST || !chestTitleState().ready)
        return false;
    const auto owned = scanPartyOwnedChestArtifacts(title, *party);
    return owned && applyActiveChestLiveOwnerHandoff(
        title, maps->pInfos[mapId].mapTreasureLevel, *table, *owned);
}

}  // namespace mm6
