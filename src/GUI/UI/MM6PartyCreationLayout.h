#pragma once

#include <array>

#include "Library/Geometry/Point.h"
#include "Library/Geometry/Rect.h"
#include "Library/Geometry/Size.h"

/**
 * MM6 party creation screen positions in 640x480 render coordinates, as the original executable lays them out.
 * Lists step 16 pixels per row, the `create.fnt` height minus two. Centered text is centered over the width named
 * next to its position. The face arrows are part of the background art, so their buttons have no textures.
 */
namespace mm6::party_creation {

constexpr int kBackgroundY = 23; // makeme.pcx is 640x457 and sits under the 23-pixel maketop strip.

constexpr Pointi kClassName = {85, 97}; // Left-aligned, columns 159 apart.
constexpr int kStatY = 160;
constexpr int kStatButtonY = 162;
constexpr int kSkillsHeadingY = 289;
constexpr int kSkillY = 308;

constexpr std::array<int, 4> kFacePrevX = {86, 245, 403, 562};
constexpr std::array<int, 4> kFaceNextX = {118, 277, 435, 594};
constexpr int kFaceButtonY = 31;
constexpr Sizei kFaceButtonSize = {32, 16};

constexpr int kFocusArrowLeftGap = 14; // arrowl is drawn this far left of the focused row.
constexpr int kFocusArrowRightGap = 5; // arrowr is drawn this far left of the focused row's right edge.

constexpr Pointi kClassHeading = {37, 398}; // Centered over 193.
constexpr std::array<int, 2> kClassColumnX = {60, 140};
constexpr int kClassColumnWidth = 70;
constexpr int kListY = 417;

constexpr Pointi kAvailableSkillsHeading = {238, 398}; // Centered over 236.
constexpr int kAvailableSkillsX = 230;
constexpr int kAvailableSkillsColumnWidth = 80;

constexpr Pointi kBonusHeading = {498, 394}; // Centered over 92.
constexpr Pointi kBonusValue = {502, 410}; // Centered over 84.

constexpr Recti kMinusButton = {482, 392, 20, 35};
constexpr Pointi kMinusPressed = {485, 408};
constexpr Recti kPlusButton = {580, 392, 22, 35};
constexpr Pointi kPlusPressed = {588, 405};
constexpr Recti kOkButton = {511, 438, 63, 29};
constexpr Pointi kOkPressed = {511, 439};

} // namespace mm6::party_creation
