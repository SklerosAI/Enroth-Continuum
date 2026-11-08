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
 * native slots, ACTR and EVRT bindings that every later capture keeps.
 *
 * @param sourceV4                      First-map v4 anchor of the admitted session.
 * @param title                         Must be MM6.
 */
void armFirstMapManualSave(std::span<const std::uint8_t> sourceV4, GameTitle title);
void disarmFirstMapManualSave();
bool isFirstMapManualSaveArmed();

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
 * Saves the armed first-map session from the idle paused Save menu.
 *
 * @param fileName                      Selected `saveNNN.ec6` slot, or empty for the first free numbered slot.
 * @param displayName                   Title shown in the save list.
 * @return                              Outcome and the slot it applies to. Never throws.
 */
FirstMapManualSaveResult saveFirstMapManualSlot(std::string_view fileName, std::string_view displayName);

/**
 * @return                              Loadable numbered first-map slots of the startup MM6 profile, sorted by file
 *                                      name. Empty when there is no profile root or the saves folder is refused.
 */
std::vector<SaveSlotInfo> listFirstMapManualSlots();

/**
 * @param fileName                      Listed `saveNNN.ec6` slot.
 * @return                              Validated primary bytes of the slot. Throws when it can't be loaded as it is.
 */
std::vector<std::uint8_t> readFirstMapManualSlot(std::string_view fileName);

/**
 * @param fileName                      Slot selected in the main-menu Load menu.
 * @return                              Whether `loadFirstMapManualSlot` can take it: a numbered slot, with a native
 *                                      MM6 profile root to read it from.
 */
bool mayLoadFirstMapManualSlot(std::string_view fileName);

/**
 * Starts the main-menu load of a numbered first-map slot. Reads and stages the slot, puts the load attempt on the
 * engine and publishes its owners. World preparation and the attempt's release follow in the game loop. The slot
 * file is only read.
 *
 * @param fileName                      Listed `saveNNN.ec6` slot. Throws when it can't be loaded, the caller then
 *                                      abandons the load.
 */
void loadFirstMapManualSlot(std::string_view fileName);

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
