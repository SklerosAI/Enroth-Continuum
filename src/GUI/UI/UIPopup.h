#pragma once

#include <string>

#include "Engine/Objects/ItemEnums.h"

#include "Library/Geometry/Point.h"

struct Item;

void DrawPopupWindow(int uX, int uY, int uWidth, int uHeight);  // idb

/**
 * If `mousePos` is over a character portrait, uses the picked item on that character and return true. Note that using
 * an item can either consume that item, or display an error string (E.g. "Crossbow can not be used that way").
 *
 * @param mousePos                  Mouse position in 640x480 UI coordinates.
 * @return                          Whether the picked item was consumed.
 */
bool tryUseItemOnPortrait(Pointi mousePos);

/**
 * Right-click dispatcher for the current screen. Called every frame from `GUI_UpdateWindows` while
 * `holdingMouseRightButton` is set, as its last step, so the resulting popup lands on top of every
 * window's `Update()` output.
 *
 * Not every branch is pure rendering. Does potion mixing, identification, repair, plays the monster-id speech reaction.
 *
 * @param mousePos                  Mouse position in 640x480 UI coordinates.
 * @offset 0x00416D62
 */
void UI_OnMouseRightClick(Pointi mousePos);

/**
 * Draws the info box of an item next to the mouse. The active character first tries to identify and repair the item
 * with his skills.
 *
 * @param inspect_item                  Item to show.
 * @offset 0x0041D895
 */
void GameUI_DrawItemInfo(Item *inspect_item);

/**
 * Draws the description of the spell a spellbook teaches, with its mastery lines and mana cost.
 *
 * @param spellbook                     Spellbook item.
 * @offset 0x4B1523
 */
void showSpellbookInfo(ItemId spellbook);

class GraphicsImage;

extern GraphicsImage *parchment;
extern GraphicsImage *messagebox_corner_x;       // 5076AC
extern GraphicsImage *messagebox_corner_y;       // 5076B4
extern GraphicsImage *messagebox_corner_z;       // 5076A8
extern GraphicsImage *messagebox_corner_w;       // 5076B0
extern GraphicsImage *messagebox_border_top;     // 507698
extern GraphicsImage *messagebox_border_bottom;  // 5076A4
extern GraphicsImage *messagebox_border_left;    // 50769C
extern GraphicsImage *messagebox_border_right;   // 5076A0

extern bool holdingMouseRightButton;
extern bool rightClickItemActionPerformed;
extern bool identifyOrRepairReactionPlayed;
extern bool monsterIdReactionPlayed;
