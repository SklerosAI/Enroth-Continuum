#pragma once

#include "Library/Geometry/Point.h"
#include "Library/Geometry/Rect.h"

// Escape menu buttons. The art is the `options` panel at (8, 8), each pressed image sits at its button's top left.
inline constexpr Recti mm6GameMenuResumeRect = {18, 160, 219, 39};
inline constexpr Recti mm6GameMenuNewRect = {18, 213, 219, 39};
inline constexpr Recti mm6GameMenuSaveRect = {18, 266, 219, 39};
inline constexpr Recti mm6GameMenuControlsRect = {242, 160, 215, 39};
inline constexpr Recti mm6GameMenuLoadRect = {242, 213, 215, 39};
inline constexpr Recti mm6GameMenuQuitRect = {242, 266, 215, 39};

// The Controls screen, `ControlBG` at (8, 132) under the `options` panel.
inline constexpr Recti mm6ControlsTurn16Rect = {20, 261, 74, 18};
inline constexpr Recti mm6ControlsTurn32Rect = {95, 261, 74, 18};
inline constexpr Recti mm6ControlsTurnSmoothRect = {170, 261, 74, 18};
inline constexpr Pointi mm6ControlsTurn16Mark = {21, 262};
inline constexpr Pointi mm6ControlsTurn32Mark = {96, 262};
inline constexpr Pointi mm6ControlsTurnSmoothMark = {171, 262};
inline constexpr Pointi mm6ControlsDetailHighMark = {171, 212};
inline constexpr Recti mm6ControlsWalkSoundRect = {21, 293, 224, 16};
inline constexpr Recti mm6ControlsShowDamageRect = {21, 312, 224, 16};
inline constexpr Recti mm6ControlsResumeRect = {261, 292, 194, 37};

inline constexpr int mm6ControlsSoundY = 161;
inline constexpr int mm6ControlsMusicY = 212;
inline constexpr int mm6ControlsVoiceY = 262;
inline constexpr int mm6ControlsLeftArrowX = 262;
inline constexpr int mm6ControlsRightArrowX = 438;
inline constexpr int mm6ControlsSliderX = 278; // Level n is marked at x 278 + 16 n, a click picks the notch under it.
inline constexpr int mm6ControlsSliderNotch = 16;
