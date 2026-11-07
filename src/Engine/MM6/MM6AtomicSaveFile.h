#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "MM6SaveMetadata.h"

namespace mm6 {

// The caller must pass the resolved native MM6 user-profile root, not a ufs
// display path. RAM and custom FileSystem implementations have no atomic rename.
// These provisional v1 helpers require the eighteen-section KnownSaveFields
// candidate; they do not validate private map identity or complete live state.
void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container);

// Separate EC6 v2 raw-item/SHOP slot boundary. A v1 slot cannot be overwritten
// or recovered as v2 implicitly. Structural validation here must be followed
// by full map/base and runtime-owner staging before a live load.
void writeAtomicRawSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                            std::span<const std::uint8_t> container);

// Standalone known-field recovery only; do not call from live SaveLoad until
// the full map and runtime transaction is ready. Returns no value when neither
// primary nor backup exists. A damaged primary is restored from a valid backup
// before bytes are returned. Throws if neither candidate is valid.
std::optional<std::vector<std::uint8_t>> loadRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName);
std::optional<std::vector<std::uint8_t>> loadRecoverableRawSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName);

struct SaveSlotInfo {
    std::string fileName;
    SaveHeaderFields header;
    bool needsRecovery = false;
};

// Structural-only inventory. It is not a playable-slot list: a well-formed
// .ec6 candidate can still fail the full gameplay stage. A recovery-needed
// entry must be loaded through the matching recovery helper.
std::vector<SaveSlotInfo> listKnownSaveSlots(const std::filesystem::path &profileRoot);
std::vector<SaveSlotInfo> listRawSaveSlots(const std::filesystem::path &profileRoot);

struct SaveFileCandidate {
    std::vector<std::uint8_t> bytes;
    bool needsRecovery = false;
};

using SaveCandidateValidator = std::function<void(std::span<const std::uint8_t>)>;

// Select a primary or backup without restoring it. Both candidates must pass
// known-section decoding and the caller's read-only semantic validation. A future
// live load must finish state staging before any recovery or global mutation.
std::optional<SaveFileCandidate> preflightRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate);
std::optional<SaveFileCandidate> preflightRecoverableRawSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate);

// Detached EC6 v3 boundary. The validator must stage every gameplay owner,
// not merely decode the 22-section manifest. V3 refuses a recognizable
// foreign-version primary or backup before any write or recovery; the read
// and list paths do not repair disk.
void writeAtomicV3SaveFile(const std::filesystem::path &profileRoot,
                           std::string_view slotName,
                           std::span<const std::uint8_t> container,
                           const SaveCandidateValidator &validate);

// Test-only v4 candidate writer. It accepts only candidate-v4.ec6 or
// candidate-v4-after-action.ec6 in an
// owner-only mm6-restart-* directory immediately under the system temp root.
// The required callback must stage all live v4 owners before any file change.
void writeAtomicV4SaveFileForTest(const std::filesystem::path &profileRoot,
                                  std::string_view slotName,
                                  std::span<const std::uint8_t> container,
                                  const SaveCandidateValidator &validate);
void writeAtomicV5SaveFileForTest(const std::filesystem::path &profileRoot,
                                  std::string_view slotName,
                                  std::span<const std::uint8_t> container,
                                  const SaveCandidateValidator &validate);

// First-map v5 result policy for the two existing disposable candidate slots.
// The caller supplies exact observed primary/backup bytes before and after an
// atomic attempt, plus whether the writer returned normally. A failed write
// never reports success: a new primary after failure needs independent
// read-only preflight before any retry or gameplay admission.
struct V5FirstMapSlotState {
    std::optional<std::vector<std::uint8_t>> primary;
    std::optional<std::vector<std::uint8_t>> backup;
};

enum class V5FirstMapWriteResult {
    NoNewPrimaryNeedsPreflight,
    Committed,
    NewPrimaryUnconfirmedNeedsPreflight,
};

V5FirstMapWriteResult assessV5FirstMapWriteResultForTest(
    GameTitle title, std::string_view slotName,
    std::span<const std::uint8_t> proposed,
    const V5FirstMapSlotState &before,
    const V5FirstMapSlotState &after,
    bool writerReturned,
    const SaveCandidateValidator &validate);

// Select a valid first-map primary or backup without repairing disk. A
// recognizable foreign version or valid v5 file for another map blocks the
// entire slot. Neither this API nor the result policy enters normal SaveLoad.
std::optional<SaveFileCandidate> preflightRecoverableV5FirstMapSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    GameTitle title, const SaveCandidateValidator &validate);
// Outdoor v6 is a detached test candidate with active v5 plus VIST. Only the
// exact disposable slot is admitted; both write and read need a full semantic
// callback. Neither function publishes a world or repairs a save slot.
void writeAtomicV6OutdoorSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate);
std::optional<SaveFileCandidate> preflightRecoverableV6OutdoorSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate);
std::optional<SaveFileCandidate> preflightRecoverableV3SaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate);
std::vector<SaveSlotInfo> listV3SaveSlotsWithPreflight(
    const std::filesystem::path &profileRoot, const SaveCandidateValidator &validate);

// Read-only candidate inventory for a future MM6 load menu. Uses the same
// semantic validator and primary/backup selection as the eventual load. The
// caller must supply a complete gameplay admission validator before exposing
// these entries in UI; a structural or partial validator is insufficient.
std::vector<SaveSlotInfo> listRawSaveSlotsWithPreflight(
    const std::filesystem::path &profileRoot, const SaveCandidateValidator &validate);

namespace detail {

struct SaveSlotRollbackSnapshot {
    std::optional<std::vector<std::uint8_t>> primary;
    std::optional<std::vector<std::uint8_t>> backup;
};

// Test-only transaction support: snapshot the exact existing primary/backup
// bytes and restore both names after a later map-entry failure. This does not
// make arbitrary world mutation atomic.
SaveSlotRollbackSnapshot snapshotSaveSlotForRollback(
    const std::filesystem::path &profileRoot, std::string_view slotName);
void restoreSaveSlotForRollback(const std::filesystem::path &profileRoot,
                               std::string_view slotName,
                               const SaveSlotRollbackSnapshot &snapshot);

enum class AtomicSaveStep {
    BeforeTempWrite,
    AfterPartialTempWrite,
    BeforeBackupReplace,
    BeforePrimaryReplace,
    AfterPrimaryReplaceBeforeDirectorySync,
};
using AtomicSaveFault = std::function<void(AtomicSaveStep)>;

// Deterministic fault seam for the filesystem unit tests.
void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container, const AtomicSaveFault &fault);
void writeAtomicRawSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                            std::span<const std::uint8_t> container, const AtomicSaveFault &fault);
void writeAtomicV3SaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                           std::span<const std::uint8_t> container,
                           const SaveCandidateValidator &validate,
                           const AtomicSaveFault &fault);
void writeAtomicV4SaveFileForTest(const std::filesystem::path &profileRoot,
                                  std::string_view slotName,
                                  std::span<const std::uint8_t> container,
                                  const SaveCandidateValidator &validate,
                                  const AtomicSaveFault &fault);
void writeAtomicV5SaveFileForTest(const std::filesystem::path &profileRoot,
                                  std::string_view slotName,
                                  std::span<const std::uint8_t> container,
                                  const SaveCandidateValidator &validate,
                                  const AtomicSaveFault &fault);
void writeAtomicV6OutdoorSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault);

} // namespace detail
} // namespace mm6
