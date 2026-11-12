#pragma once

#include <array>

#include "Engine/Spells/SpellEnums.h"

class GUIWindow;
class GraphicsImage;

/**
 * What the two MM6 quick spell hot spots do, carried as the message parameter of
 * `UIMSG_ClickInstallRemoveQuickSpellBtn` and `UIMSG_HintSelectRemoveQuickSpellBtn`.
 */
enum class MM6QuickSpellAction {
    MM6_QUICK_SPELL_ACTION_SET = 0, // TABSPELL.
    MM6_QUICK_SPELL_ACTION_REMOVE = 1, // The school header picture.
};
using enum MM6QuickSpellAction;

/**
 * @param school                        Spellbook page.
 * @return                              The school header picture `<school>000` at index 0, then the pictures of
 *                                      the school's eleven spells `<school>001..011`.
 */
std::array<GraphicsImage *, 12> loadMM6SpellbookPage(MagicSchool school);

/**
 * Creates the MM6 spellbook buttons for the active character's current page: all eleven spell cells, the school
 * tabs the character has a skill for, TABSPELL, the header picture and TABEXIT.
 *
 * @param window                        Spellbook window.
 */
void createMM6SpellbookButtons(GUIWindow *window);

/**
 * Draws the MM6 spellbook for the active character's current page.
 *
 * @param page                          Art from `loadMM6SpellbookPage` for that page.
 */
void drawMM6Spellbook(const std::array<GraphicsImage *, 12> &page);

/**
 * Handles a click on TABSPELL or on the header picture.
 *
 * @param action                        Which of the two was clicked.
 */
void clickMM6QuickSpellButton(MM6QuickSpellAction action);

/**
 * Shows the status bar hint for TABSPELL or the header picture.
 *
 * @param action                        Which of the two is under the mouse.
 */
void hintMM6QuickSpellButton(MM6QuickSpellAction action);
