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
// These helpers require the current sixteen-section KnownSaveFields candidate;
// they do not validate the private map identity or complete live state.
void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container);

// Standalone known-field recovery only; do not call from live SaveLoad until
// the full map and runtime transaction is ready. Returns no value when neither
// primary nor backup exists. A damaged primary is restored from a valid backup
// before bytes are returned. Throws if neither candidate is valid.
std::optional<std::vector<std::uint8_t>> loadRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName);

struct SaveSlotInfo {
    std::string fileName;
    SaveHeaderFields header;
    bool needsRecovery = false;
};

// Read-only inventory for a future MM6 save menu. Only known-section .ec6
// primaries or recoverable backups are listed; malformed slots are omitted.
// A recovery-needed entry must be loaded through loadRecoverableSaveFile.
std::vector<SaveSlotInfo> listKnownSaveSlots(const std::filesystem::path &profileRoot);

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

namespace detail {

enum class AtomicSaveStep {
    BeforeTempWrite,
    AfterPartialTempWrite,
    BeforeBackupReplace,
    BeforePrimaryReplace,
};
using AtomicSaveFault = std::function<void(AtomicSaveStep)>;

// Deterministic fault seam for the filesystem unit tests.
void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container, const AtomicSaveFault &fault);

} // namespace detail
} // namespace mm6
