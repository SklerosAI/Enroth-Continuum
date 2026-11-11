#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "MM6AtomicSaveFile.h"
#include "MM6V5FirstMapSaveWriter.h"

namespace mm6 {

// The caller must supply the native MM6 path resolved at startup, including
// any explicit --user-path override. A display path or an inferred home path
// is not an authority for a production slot.
struct V5FirstMapNativeRoot {
    std::filesystem::path startupResolvedMm6Root;
    std::filesystem::path requestedRoot;
};

/**
 * @param slotName                      File name to check.
 * @return                              Whether it names a numbered manual slot, `saveNNN.ec6`.
 */
bool isNumberedV5FirstMapSlot(std::string_view slotName);

// Read-only admission for one numbered manual slot. All existing primary and
// backup files must be exact, semantically valid v7 saves or first-map v5
// candidates; damaged, foreign-version and backup-only slots refuse writes.
V5FirstMapSlotState preflightV5FirstMapNativeWriteSlot(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate);

// Detached test seam for the future native write policy. It never captures a
// live world or touches SaveLoad/UI. Tests use disposable native directories.
// A non-committed result always requires read-only preflight before retry.
V5FirstMapSaveOutcome writeV5FirstMapNativeCandidateForTest(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate,
    const detail::AtomicSaveFault &fault = {});

/**
 * Commits a candidate to one numbered slot, keeping the old primary as the backup. The candidate and every file of
 * the slot must be a v7 save, or a v5 save on the first map, and pass `validate`. Every check runs before the first
 * file change.
 *
 * @param root                          Startup-resolved MM6 profile root.
 * @param slotName                      Numbered slot, `saveNNN.ec6`.
 * @param candidate                     Encoded save.
 * @param title                         Must be MM6.
 * @param validate                      Full semantic validator for one candidate.
 * @param fault                         Fault injection for tests, empty otherwise.
 * @return                              Atomic outcome. Anything but `Committed` leaves the slot for read-only
 *                                      preflight before a retry. Throws when refused before writing.
 */
V5FirstMapSaveOutcome writeNumberedSlotCandidate(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate,
    const detail::AtomicSaveFault &fault = {});

/**
 * Captures the live first-map session at the idle paused Save menu and commits it to one numbered slot. The
 * capture, title and slot checks all run before the first file change, and throw on refusal.
 *
 * @param root                          Startup-resolved MM6 profile root.
 * @param slotName                      Numbered slot, `saveNNN.ec6`.
 * @param sourceV4                      First-map v4 anchor of the running session.
 * @param displayName                   Title shown in the save list.
 * @param title                         Must be MM6.
 * @param validate                      Full semantic validator for the candidate and the files of the slot.
 * @param fault                         Fault injection for tests, empty otherwise.
 * @return                              Written candidate and the atomic outcome. Anything but `Committed` leaves
 *                                      the slot for read-only preflight before a retry.
 */
V5FirstMapSaveAttempt writeV5FirstMapNativeLiveSave(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> sourceV4, std::string_view displayName,
    GameTitle title, const SaveCandidateValidator &validate, const detail::AtomicSaveFault &fault = {});

/**
 * Read-only list of the numbered slots under the startup MM6 root. Disk is never repaired.
 *
 * @param root                          Startup-resolved MM6 profile root.
 * @param title                         Must be MM6.
 * @param validate                      Full semantic validator for one candidate.
 * @return                              Slots whose primary is a v7 save, or a v5 save on the first map, and passes
 *                                      `validate`, sorted by file name. Backup-only, damaged and foreign slots are
 *                                      left out.
 */
std::vector<SaveSlotInfo> listV5FirstMapNativeSlots(
    const V5FirstMapNativeRoot &root, GameTitle title,
    const SaveCandidateValidator &validate);

/**
 * Read-only load selection for one numbered slot. A slot that needs backup recovery is refused.
 *
 * @param root                          Startup-resolved MM6 profile root.
 * @param slotName                      Numbered slot, `saveNNN.ec6`.
 * @param title                         Must be MM6.
 * @param validate                      Full semantic validator for the candidate.
 * @return                              Primary bytes that passed `validate`. Throws for a missing or invalid slot.
 */
std::vector<std::uint8_t> readV5FirstMapNativeLoadSlot(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    GameTitle title, const SaveCandidateValidator &validate);

} // namespace mm6
