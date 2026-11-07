#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "MM6AtomicSaveFile.h"
#include "MM6V5LiveSaveCapture.h"

namespace mm6 {

// This first-map service has no normal SaveLoad or menu entry point. A result
// other than Committed requires read-only slot preflight before any retry.
enum class V5FirstMapSaveOutcome {
    Committed,
    NoNewPrimaryNeedsPreflight,
    IndeterminateNeedsPreflight,
};

struct V5FirstMapSaveAttempt {
    std::vector<std::uint8_t> candidate;
    V5FirstMapSaveOutcome outcome;
};

// Capture exact MM6 first-map owners at the selected live boundary from a
// source-anchored v4 save, then write only an owner-only disposable v5 slot.
V5FirstMapSaveAttempt writeV5FirstMapLiveSaveForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> sourceV4, GameTitle title,
    V5FirstMapCaptureBoundary boundary = V5FirstMapCaptureBoundary::SettledInput);

namespace detail {

// Narrow disk seam for fault tests. The caller must provide a complete
// first-map semantic validator; the live service supplies detached staging.
V5FirstMapSaveOutcome writeV5FirstMapCandidateForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault = {});

} // namespace detail
} // namespace mm6
