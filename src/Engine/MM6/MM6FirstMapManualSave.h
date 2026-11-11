#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6AtomicSaveFile.h"
#include "MM6V5LiveSaveCapture.h"

namespace mm6 {

/**
 * Sets the native MM6 profile root that startup resolved, including an explicit `--user-path`. A RAM user
 * filesystem leaves manual saves without a destination.
 *
 * @param ramFsUserData                 Whether the user filesystem lives in memory.
 * @param userPath                      Native user path passed to the user filesystem.
 */
void setStartupNativeProfileRoot(bool ramFsUserData, const std::filesystem::path &userPath);

/**
 * Arms manual saving for a first-map session that a load route has admitted to input. The anchor carries the
 * native slots, ACTR and EVRT bindings that every later first-map capture keeps, and its raw owners stay the session's
 * template for saves on later maps.
 *
 * @param sourceV4                      First-map v4 anchor of the admitted session.
 * @param title                         Must be MM6.
 */
void armFirstMapManualSave(std::span<const std::uint8_t> sourceV4, GameTitle title);

/**
 * Drops the first-map anchor when the party leaves a map. Saving stays armed, later saves are v7 saves.
 */
void leaveFirstMapManualSave();

/**
 * Ends manual saving for the session, for a new or loaded game.
 */
void disarmFirstMapManualSave();

/**
 * @return                              Whether the session still has its first-map anchor, so that the next save is a
 *                                      v5 save on the first map.
 */
bool isFirstMapManualSaveArmed();

/**
 * @return                              Whether the session can be saved at all: a staged new game, or a load.
 */
bool isManualSaveArmed();

enum class ManualSaveOutcome {
    MANUAL_SAVE_COMMITTED,    // The new primary and backup are durably in place.
    MANUAL_SAVE_REFUSED,      // Nothing was written: no armed session, no profile root, or an unsupported live state.
    MANUAL_SAVE_NOT_WRITTEN,  // The write failed before the primary was replaced, the old slot is unchanged.
    MANUAL_SAVE_UNCONFIRMED,  // The write failed after the primary may have been replaced.
};
using enum ManualSaveOutcome;

struct FirstMapManualSaveResult {
    ManualSaveOutcome outcome = MANUAL_SAVE_REFUSED;
    std::string fileName; // Slot that was attempted, empty when refused before slot selection.
};

/**
 * Saves the armed session from the idle paused Save menu: as a v5 save while the party is still on the first map of
 * the session, and as a v7 save on any map once it has left it.
 *
 * @param fileName                      Selected `saveNNN.ec6` slot, or empty for the first free numbered slot.
 * @param displayName                   Title shown in the save list.
 * @return                              Outcome and the slot it applies to. Never throws.
 */
FirstMapManualSaveResult saveFirstMapManualSlot(std::string_view fileName, std::string_view displayName);

/**
 * Saves the armed session to one slot of the profile, as `saveFirstMapManualSlot` does.
 *
 * @param fileName                      Slot of the profile, see `isMM6SaveSlot`.
 * @param displayName                   Title shown in the save list.
 * @param boundary                      Where a save on the first map is captured, the idle paused Save menu or settled
 *                                      input.
 * @return                              Outcome and the slot it applies to. Never throws.
 */
FirstMapManualSaveResult saveSessionSlot(std::string_view fileName, std::string_view displayName,
                                         V5FirstMapCaptureBoundary boundary);

/**
 * @return                              Loadable slots of the startup MM6 profile, sorted by file name. Empty
 *                                      when there is no profile root or the saves folder is refused.
 */
std::vector<SaveSlotInfo> listFirstMapManualSlots();

/**
 * @param fileName                      Listed slot.
 * @return                              Validated primary bytes of the slot. Throws when it can't be loaded as it is.
 */
std::vector<std::uint8_t> readFirstMapManualSlot(std::string_view fileName);

/**
 * @param fileName                      Slot selected in the main-menu Load menu.
 * @return                              Whether `loadFirstMapManualSlot` can take it: a slot of the profile, see
 *                                      `isMM6SaveSlot`, with a native MM6 profile root to read it from.
 */
bool mayLoadFirstMapManualSlot(std::string_view fileName);

/**
 * Starts the main-menu load of a slot. Reads and stages the slot and publishes its owners, a first-map v5 slot
 * through a load attempt on the engine. World preparation, and the attempt's release, follow in the game loop. A v7
 * slot arms saving for the session right away. The slot file is only read.
 *
 * @param fileName                      Listed slot. Throws when it can't be loaded, the caller then abandons the load.
 */
void loadFirstMapManualSlot(std::string_view fileName);

/**
 * Starts loading a slot over the running session. The slot is read and staged first. Then the session's parked maps
 * and its save anchor are dropped, and the slot is published as `loadFirstMapManualSlot` publishes it.
 *
 * @param fileName                      Slot of the profile, see `isMM6SaveSlot`.
 * @return                              Whether the slot was published. On `false` the slot couldn't be staged and the
 *                                      session is unchanged. Throws when publication fails, the session is then
 *                                      partial.
 */
bool loadSlotOverSession(std::string_view fileName);

namespace testing {
/**
 * GameTest stand-in for the startup root, which a RAM user filesystem leaves absent.
 *
 * @param root                          Owner-only `mm6-restart-*` directory directly under the system temp root, or
 *                                      `std::nullopt` to clear it.
 */
void setNativeProfileRootForTest(std::optional<std::filesystem::path> root);

/**
 * @param fault                         Fault injection for the following manual saves, empty to clear it.
 */
void setManualSaveFaultForTest(::mm6::detail::AtomicSaveFault fault);
} // namespace testing

} // namespace mm6
