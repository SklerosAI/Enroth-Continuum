#pragma once

#include <array>
#include <cstdint>

#include "Engine/GameTitle.h"

namespace mm6 {

// Title-local owner for the original MM6 chest LCG and 400..429 generated
// artifact band. No MM7 Party or RandomEngine field is used for this state.
// The ready bit must be set by a validated new-game or .ec6 restore path.
struct ChestTitleState {
    bool ready = false;
    std::uint32_t lcgState = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

inline bool restoreChestTitleState(GameTitle title, ChestTitleState *owner,
                                   std::uint32_t lcgState,
                                   const std::array<std::uint8_t, 30> &generatedFlags) {
    if (title != GameTitle::MM6 || !owner)
        return false;
    for (std::uint8_t flag : generatedFlags)
        if (flag > 1)
            return false;
    *owner = ChestTitleState{true, lcgState, generatedFlags};
    return true;
}

inline void resetChestTitleState(ChestTitleState *owner) {
    if (owner)
        *owner = {};
}

// Explicit MM6-only process owner. Until new-game and .ec6 paths call the
// validated restore/reset functions, its default unready state fails closed.
inline ChestTitleState &chestTitleState() {
    static ChestTitleState owner;
    return owner;
}

}  // namespace mm6
