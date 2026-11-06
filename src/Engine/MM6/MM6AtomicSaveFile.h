#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace mm6 {

// The caller must pass the resolved native MM6 user-profile root, not a ufs
// display path. RAM and custom FileSystem implementations have no atomic rename.
// These helpers require the current sixteen-section KnownSaveFields candidate;
// they do not validate the private map identity or complete live state.
void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container);

// Returns no value when neither primary nor backup exists. A damaged primary
// is restored from a valid backup before bytes are returned. Throws if neither
// candidate is valid, leaving both files untouched.
std::optional<std::vector<std::uint8_t>> loadRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName);

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
