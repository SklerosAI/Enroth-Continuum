#pragma once

#include <array>

#include "Engine/GameTitle.h"
#include "Library/Geometry/Point.h"
#include "Library/Geometry/Size.h"

inline constexpr Pointi hudPortraitFacePosition(GameTitle title, int index, Pointi mm7Position) {
    if (title != GameTitle::MM6 || index < 0 || index >= 4)
        return mm7Position;
    constexpr std::array<int, 4> x = {22, 135, 248, 360};
    return {x[index], 383};
}

// MM6's sprite blit centers aframe1 on these x coordinates and anchors its
// last row to y=467. The original sprite is 69x91, yielding y=377.
inline constexpr Pointi hudPortraitSelectionPosition(GameTitle title, int index,
        Sizei frameSize, Pointi mm7Position) {
    if (title != GameTitle::MM6 || index < 0 || index >= 4 || frameSize.w <= 0 || frameSize.h <= 0)
        return mm7Position;
    constexpr std::array<int, 4> centerX = {50, 163, 276, 388};
    return {centerX[index] - frameSize.w / 2, 467 - frameSize.h + 1};
}
