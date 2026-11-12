#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "Engine/PartyEnums.h"

#include "Library/Geometry/Rect.h"

inline constexpr int mm6HudFoodGoldY = 356;

// MM6.exe right-aligns with \r086 and \r022. GUIFont's line width leaves out the last glyph's right spacing, so the
// same ink needs a margin one larger.
inline constexpr int mm6HudFoodRightMargin = 87;
inline constexpr int mm6HudGoldRightMargin = 23;

inline constexpr Recti mm6HudFoodHoverRect = {484, 355, 30, 38}; // The apple.
inline constexpr Recti mm6HudGoldHoverRect = {603, 354, 28, 32}; // The coin stack.

inline constexpr Recti mm6HudCompassClipRect = {536, 0, 42, 480};

/**
 * @param yaw                           Party yaw, in 0..2047.
 * @return                              Top left of the COMPASS strip. It shows through the slot in TAP, which is
 *                                      drawn over it.
 */
inline constexpr Pointi mm6HudCompassPosition(int yaw) {
    return {528 - (2048 - yaw) * 15 / 128, 10};
}

inline constexpr Recti mm6HudMinimapRect = {480, 25, 152, 115};
inline constexpr Pointi mm6HudMapBackPosition = {482, 25}; // Indoors only.

/**
 * @param yaw                           Party yaw, in 0..2047.
 * @return                              Index of the MAPDIR arrow that the minimap draws for `yaw`.
 */
inline constexpr int mm6HudMinimapArrowIndex(int yaw) {
    return ((yaw - 128) & 2047) >> 8;
}

inline constexpr Pointi mm6HudZoomOutCenter = {502, 42}; // The minus mark on TAP.
inline constexpr Pointi mm6HudZoomInCenter = {595, 42}; // The plus mark on TAP.
inline constexpr int mm6HudZoomRadius = 17;

inline constexpr int mm6HudStatusTextY = 355;

/**
 * Party buff that MM6.exe shows as an animated sprite on the right panel.
 */
struct MM6HudBuffSprite {
    PartyBuff buff;
    std::string_view sprite;
    Pointi anchor; // Bottom centre of the sprite.
};

inline constexpr std::array<MM6HudBuffSprite, 7> mm6HudBuffSprites = {{
    {PARTY_BUFF_RESIST_FIRE, "i2_fire", {521, 254}},
    {PARTY_BUFF_RESIST_WATER, "i4_cold", {567, 254}},
    {PARTY_BUFF_RESIST_AIR, "i3_ltng", {543, 254}}, // Electricity.
    {PARTY_BUFF_PROTECTION_FROM_MAGIC, "i6_magic", {610, 254}},
    {PARTY_BUFF_RESIST_BODY, "i5_poisn", {588, 254}}, // Poison.
    {PARTY_BUFF_FEATHER_FALL, "i1_fall", {500, 254}},
    {PARTY_BUFF_WIZARD_EYE, "wizeye", {556, 30}}
}};

/**
 * Part of an MM6 hit point bar that MM6.exe draws. The top half of the bar is HITSFULL, the next quarter HITSHALF and
 * the last quarter HITSQTR, so the bar isn't linear in hit points.
 */
struct MM6HudHealthBar {
    std::string_view image;
    int top; // Of the whole image.
    int skip; // Rows left out at the top of the image.
};

/**
 * @param ratio                         Hit points over maximum hit points, above zero.
 * @return                              What to draw of the bar.
 */
inline MM6HudHealthBar mm6HudHealthBar(double ratio) {
    if (ratio > 0.5)
        return {"hitsfull", 383, 38 - static_cast<int>((std::min(ratio, 1.0) - 0.5) * 2 * 38)};
    if (ratio > 0.25)
        return {"hitshalf", 421, 21 - static_cast<int>((ratio - 0.25) * 4 * 21)};
    return {"hitsqtr", 442, 19 - static_cast<int>(ratio * 4 * 19)};
}

inline constexpr std::array<Pointi, 2> mm6HudHirelingPositions = {{{491, 149}, {561, 149}}};
inline constexpr Pointi mm6HudNpcLeftArrowPosition = {474, 174};
inline constexpr Pointi mm6HudNpcRightArrowPosition = {626, 174};

inline constexpr Recti mm6HudDateTimeHoverRect = {481, 0, 153, 67};
inline constexpr std::array<Pointi, 4> mm6HudPortraitCenters = {{{52, 422}, {165, 422}, {280, 422}, {390, 422}}};
inline constexpr int mm6HudPortraitRadius = 35;
inline constexpr std::array<Recti, 4> mm6HudVitalsHoverRects = {{{90, 378, 21, 89}, {203, 378, 21, 89}, {316, 378, 21, 89}, {428, 378, 21, 89}}};
