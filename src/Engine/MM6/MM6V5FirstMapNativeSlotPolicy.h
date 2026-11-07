#pragma once

#include <filesystem>
#include <span>
#include <string_view>

#include "Engine/GameTitle.h"
#include "MM6AtomicSaveFile.h"
#include "MM6V5FirstMapSaveWriter.h"

namespace mm6 {

// The caller must supply the native MM6 path resolved at startup, including
// any explicit --user-path override. A display path or an inferred home path
// is not an authority for a production slot. No UI route calls this yet.
struct V5FirstMapNativeRoot {
    std::filesystem::path startupResolvedMm6Root;
    std::filesystem::path requestedRoot;
};

// Read-only admission for one numbered manual slot. All existing primary and
// backup files must be exact, semantically valid first-map v5 candidates;
// damaged, foreign-version, other-map and backup-only slots refuse writes.
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

} // namespace mm6
