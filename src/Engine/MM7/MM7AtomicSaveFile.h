#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"

namespace mm7 {

// Startup supplies the resolved user path after initializing its user filesystem.
// RAM and other titles clear the context. The read-only result is available only
// while that path is still an ordinary native directory; callers must not use
// the result as authority to bypass the atomic file helper's own path checks.
void setStartupNativeProfileRoot(GameTitle title, bool ramFsUserData,
                                 const std::filesystem::path &userPath);
std::optional<std::filesystem::path> startupNativeProfileRoot();

// Content validation is supplied by the caller so the native file transaction
// cannot accidentally admit a save after checking only its lightweight header.
// The callback returns false for invalid bytes and lets I/O or process errors
// propagate. It must not mutate live game state.
using SaveBytesValidator = std::function<bool(std::span<const std::uint8_t>)>;

struct SaveFileCandidate {
  std::vector<std::uint8_t> bytes;
  bool fromBackup = false;
};

// Read-only selection. Returns no value if neither file exists; throws if
// neither existing candidate passes validation. A valid backup is not copied
// over the primary until a separate, explicit recovery decision is made.
std::optional<SaveFileCandidate>
preflightSaveSlot(const std::filesystem::path &profileRoot,
                  std::string_view slotName,
                  const SaveBytesValidator &validate);

// Native MM7 profile only. The previous valid primary becomes .bak before a
// new primary is installed. Corrupt existing files and backup-only slots are
// refused without modifying either file. A write error after primary rename
// has an uncertain result and must never be reported as a successful save.
void writeAtomicSaveSlot(const std::filesystem::path &profileRoot,
                         std::string_view slotName,
                         std::span<const std::uint8_t> bytes,
                         const SaveBytesValidator &validate);

namespace detail {

enum class AtomicSaveStep {
  BeforeTempWrite,
  AfterPartialTempWrite,
  BeforeBackupReplace,
  BeforePrimaryReplace,
  AfterPrimaryReplaceBeforeDirectorySync,
};
using AtomicSaveFault = std::function<void(AtomicSaveStep)>;

// Deterministic filesystem fault seam for tests; do not install in gameplay.
void writeAtomicSaveSlot(const std::filesystem::path &profileRoot,
                         std::string_view slotName,
                         std::span<const std::uint8_t> bytes,
                         const SaveBytesValidator &validate,
                         const AtomicSaveFault &fault);

} // namespace detail
} // namespace mm7
