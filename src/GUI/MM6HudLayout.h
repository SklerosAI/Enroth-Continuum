#pragma once

#include "Library/Geometry/Rect.h"

inline constexpr int mm6HudFoodGoldY = 356;

// MM6.exe right-aligns with \r086 and \r022. GUIFont's line width leaves out the last glyph's right spacing, so the
// same ink needs a margin one larger.
inline constexpr int mm6HudFoodRightMargin = 87;
inline constexpr int mm6HudGoldRightMargin = 23;

inline constexpr Recti mm6HudFoodHoverRect = {484, 355, 30, 38}; // The apple.
inline constexpr Recti mm6HudGoldHoverRect = {603, 354, 28, 32}; // The coin stack.
