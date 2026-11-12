#pragma once

#include "Library/Geometry/Point.h"
#include "Library/Geometry/Rect.h"
#include "Library/Geometry/Size.h"

/**
 * MM6 book screen positions in 640x480 render coordinates, as the original executable lays them out. A side tab's
 * hit box and `on` art sit at `kTabX`, its `off` art at `kTabOffX`.
 */
namespace mm6::books {

constexpr Pointi kBorder5Position = {7, 8};
constexpr Pointi kBorder6Position = {461, 8};
constexpr Pointi kPagePosition = {47, 22}; // quest_bg, note_bg and time_bg.
constexpr Recti kTitleFrame = {8, 8, 460, 344};

constexpr int kTabX = 415;
constexpr int kTabOffX = 418;
constexpr Sizei kTabSize = {49, 33}; // MM6 tabs are 50x34, GUIWindow::CreateButton adds one to the width and the height.

constexpr int kNextPageTabY = 13;
constexpr int kPrevPageTabY = 48;
constexpr int kZoomInTabY = 13;
constexpr int kZoomOutTabY = 48;
constexpr int kNorthTabY = 118;
constexpr int kSouthTabY = 153;
constexpr int kWestTabY = 188;
constexpr int kEastTabY = 223;
constexpr int kPotionTabY = 118;
constexpr int kFountainTabY = 153;
constexpr int kObeliskTabY = 188;
constexpr int kSeerTabY = 223;
constexpr int kMiscTabY = 258;

constexpr Pointi kMapFramePosition = {75, 22};
constexpr Pointi kMoonPosition = {266, 198};

} // namespace mm6::books
