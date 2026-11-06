#pragma once

#include <optional>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6Portraits.h"

namespace mm6 {

// A detached numeric candidate from the original MM6 face-animation path.
// variantCount must come from the matching MM6 face/kind count table cell;
// the caller must resolve the ID against the MM6 sound table before playback.
inline std::optional<int> faceAnimationSoundId(GameTitle title, int face, int soundKind,
                                               int variantIndex, int variantCount) {
    if (title != GameTitle::MM6 || !isValidPortraitFace(face) ||
        soundKind < 1 || soundKind > 44 || variantCount < 1 || variantCount > 2 ||
        variantIndex < 0 || variantIndex >= variantCount) {
        return std::nullopt;
    }

    return 5000 + 100 * face + 2 * soundKind + variantIndex;
}

}  // namespace mm6
