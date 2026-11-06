#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "Engine/MM6/MM6ChestBaseItemPicker.h"

namespace mm6 {

// Four original scan windows, each with 581 consecutive 28-byte records.
// The bases are only 5,660 bytes apart, so the windows overlap and extend
// beyond four character strides. This is an opaque synthetic memory image,
// not a validated Character inventory view.
using DirectArtifactScanMemory = std::array<std::uint8_t, 3 * 5660 + 581 * 28>;

struct DirectArtifactPick {
    int itemId = 0;  // Zero when no candidate remains; destination was cleared.
    int candidateCount = 0;
    std::uint32_t nextRngState = 0;
    int drawsConsumed = 0;
};

// Explicit deterministic compatibility policy for a future live adapter.
// It intentionally differs from the original direct picker: it uses real
// title-specific ownership flags, excludes ID 430, applies a global union cap
// of 13, and stages a generated flag after a successful grant.
struct SafeDirectArtifactPick {
    int itemId = 0;
    int candidateCount = 0;
    bool capped = false;
    std::uint32_t nextRngState = 0;
    int drawsConsumed = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

inline std::optional<SafeDirectArtifactPick> pickDirectChestArtifactSafePolicy(
    GameTitle title, const std::array<std::uint8_t, 30> &partyOwnedFlags,
    const std::array<std::uint8_t, 30> &generatedArtifactFlags, std::uint32_t rngState) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    int occupiedCount = 0;
    for (int i = 0; i < 30; ++i) {
        if (partyOwnedFlags[i] > 1 || generatedArtifactFlags[i] > 1)
            return std::nullopt;
        occupiedCount += partyOwnedFlags[i] || generatedArtifactFlags[i];
    }
    SafeDirectArtifactPick result;
    result.generatedArtifactFlags = generatedArtifactFlags;
    result.nextRngState = rngState;
    if (occupiedCount >= 13) {
        result.capped = true;
        return result;
    }

    std::array<int, 30> candidates = {};
    for (int i = 0; i < 30; ++i)
        if (!partyOwnedFlags[i] && !generatedArtifactFlags[i])
            candidates[result.candidateCount++] = 400 + i;
    if (result.candidateCount == 0)
        return result;

    const int roll = nextChestRandom15(result.nextRngState) % result.candidateCount;
    result.drawsConsumed = 1;
    result.itemId = candidates[roll];
    result.generatedArtifactFlags[result.itemId - 400] = 1;
    return result;
}

// A bounded safe subset of the original tier-seven direct picker. The PE
// marks its local candidate mask by scan *slot index*, despite testing the
// record's item ID. An artifact-bearing record outside slots 400..430 writes
// outside that local mask in the original; reject it rather than model stack
// corruption. The caller must supply every byte in the overlapping scan span.
inline std::optional<DirectArtifactPick> pickDirectChestArtifactDetached(
    GameTitle title, const DirectArtifactScanMemory &memory, std::uint32_t rngState) {
    if (title != GameTitle::MM6)
        return std::nullopt;

    std::array<bool, 31> maskedCandidates = {};
    for (int character = 0; character < 4; ++character) {
        for (int slot = 0; slot < 581; ++slot) {
            const int offset = character * 5660 + slot * 28;
            const std::uint32_t itemId = static_cast<std::uint32_t>(memory[offset]) |
                                         (static_cast<std::uint32_t>(memory[offset + 1]) << 8) |
                                         (static_cast<std::uint32_t>(memory[offset + 2]) << 16) |
                                         (static_cast<std::uint32_t>(memory[offset + 3]) << 24);
            if (itemId >= 400 && itemId <= 429) {
                if (slot < 400 || slot > 430)
                    return std::nullopt;
                maskedCandidates[slot - 400] = true;
            }
        }
    }

    std::array<int, 31> candidates = {};
    DirectArtifactPick result;
    result.nextRngState = rngState;
    for (int id = 400; id <= 430; ++id)
        if (!maskedCandidates[id - 400])
            candidates[result.candidateCount++] = id;
    if (result.candidateCount == 0)
        return result;

    const int roll = nextChestRandom15(result.nextRngState) % result.candidateCount;
    result.drawsConsumed = 1;
    result.itemId = candidates[roll];
    return result;
}

}  // namespace mm6
