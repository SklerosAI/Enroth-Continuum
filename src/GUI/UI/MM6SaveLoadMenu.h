#pragma once

#include <string_view>
#include <vector>

#include "GUI/UI/UISaveLoad.h"

/**
 * @return                              Loadable slots from the native MM6 profile, sorted by display name with file
 *                                      name as a tie-breaker. The autosave and quick saves show their titles in the
 *                                      current language. MM6 saves never live in the user filesystem that holds MM7
 *                                      saves.
 */
std::vector<SavegameSlot> mm6LoadMenuSlots();

/**
 * @param fileName                      Slot file name.
 * @return                              Whether it is the MM6 autosave or a quick save, which the Save menu doesn't
 *                                      offer.
 */
bool isMM6AutoOrQuickSaveSlot(std::string_view fileName);

/**
 * Saves the running MM6 session from the idle Save menu. The menu closes only when the slot is committed, any
 * other outcome is reported on the status bar and keeps it open.
 *
 * @param fileName                      Selected `saveNNN.ec6` slot, or empty for a new slot.
 * @param title                         Save title to display in the save list.
 */
void mm6SaveFromMenu(std::string_view fileName, std::string_view title);

/**
 * @param fileName                      Slot selected in the main-menu Load menu.
 * @return                              Whether the slot may start loading. A refusal plays the error sound.
 */
bool mm6MayLoadFromMainMenu(std::string_view fileName);

/**
 * Refuses loading an MM6 slot over a running game, an MM6 slot loads only into a fresh session.
 */
void mm6RefuseInGameLoad();

/**
 * Writes the MM6 autosave. A session that can't be saved is skipped.
 */
void mm6AutoSave();
