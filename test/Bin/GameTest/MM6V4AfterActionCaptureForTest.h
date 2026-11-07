#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace mm6::testing {

// Called on the game thread only after a strict v4 first-map reload has
// reached input and completed one bounded movement action. Writes a distinct
// owner-only temp candidate after exact live-owner and source-backed checks.
void writeV4AfterActionCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> sourceCandidate);
void writeV5AfterActionCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> sourceCandidate);

} // namespace mm6::testing
