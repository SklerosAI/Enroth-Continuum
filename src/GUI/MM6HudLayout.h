#pragma once

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
