#pragma once

#include <vector>

#include "GUI/GUIEnums.h"

#include "Library/Geometry/Point.h"

class Character;
class GUIWindow;
class GraphicsImage;

inline constexpr Pointi MM6_CHARACTER_TAB_BACKGROUND_POSITION = {17, 14}; // fr_stats, fr_skill and fr_award.
inline constexpr Pointi MM6_INVENTORY_BACKGROUND_POSITION = {14, 17}; // fr_inven, on the 32 px item grid.
inline constexpr int MM6_AWARD_SPACING = 4;
inline constexpr Pointi MM6_DETAIL_BACKGROUND_POSITION = {481, 0};
inline constexpr Pointi MM6_DETAIL_TOGGLE_POSITION = {527, 300}; // Detail mode toggle, drawn with guy_up.

/**
 * Creates the MM6 stats, skills, inventory, awards and exit buttons at proposed positions and stores them in
 * the shared `pCharacterScreen_*Btn` pointers. Each button carries its normal and pressed MM6 art.
 *
 * @param window                        Character screen window.
 */
void createMM6CharacterScreenButtons(GUIWindow *window);

/**
 * Draws what MM6 puts under every character screen tab: the leather, the side borders and the tab button pictures.
 * MM6 tab backgrounds don't include the button strip.
 */
void drawMM6CharacterScreenUnderlay();

/**
 * Draws the highlight over the active tab's button.
 *
 * @param activeTab                     Active character screen tab.
 */
void drawMM6CharacterScreenActiveTab(WindowType activeTab);

/**
 * Draws the side borders again, over the paperdoll.
 */
void drawMM6CharacterScreenBorders();

/**
 * Draws the MM6 stats tab: five resistances instead of MM7's six, and MM6's own text positions.
 *
 * @param window                        Character screen window.
 * @param character                     Character to show.
 */
void drawMM6StatsTab(GUIWindow *window, Character *character);

/**
 * @return                              Normal and pressed MM6 exit button art.
 */
std::vector<GraphicsImage *> mm6CharacterScreenExitTextures();
