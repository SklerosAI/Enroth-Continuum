#pragma once

#include <array>
#include <stdexcept>

#include "Engine/GameTitle.h"

#include "MM6SaveMetadata.h"

namespace mm6 {

// Saved actor PIDs are bounded by the saved delta, but DLV/DDM reload can
// replace that delta or alter actors before VISU would be published. Until a
// post-reconstruction identity proof exists, only inactive VISU slots may
// cross this detached publication boundary.
inline void requireNoActiveSavedActorOverlays(
    const std::array<SaveOverlayField, 50> &overlays, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 actor PID policy requires MM6 title");
    for (const SaveOverlayField &overlay : overlays)
        if (overlay.animationLength > 0)
            throw std::runtime_error("MM6 active saved overlay lacks rebuilt actor identity");
}

} // namespace mm6
