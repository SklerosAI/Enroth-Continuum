#pragma once

#include <vector>

#include "GUI/GUIEnums.h"

class GUIWindow;
class GraphicsImage;

/**
 * Creates the MM6 stats, skills, inventory, awards and exit buttons at their original positions and stores them in
 * the shared `pCharacterScreen_*Btn` pointers. Each button carries its normal and pressed MM6 art.
 *
 * @param window                        Character screen window.
 */
void createMM6CharacterScreenButtons(GUIWindow *window);

/**
 * Draws the MM6 character screen buttons with the highlight over the active tab. MM6 tab backgrounds don't include
 * the button strip.
 *
 * @param activeTab                     Active character screen tab.
 */
void drawMM6CharacterScreenButtons(WindowType activeTab);

/**
 * @return                              Normal and pressed MM6 exit button art.
 */
std::vector<GraphicsImage *> mm6CharacterScreenExitTextures();
