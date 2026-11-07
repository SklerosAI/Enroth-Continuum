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
// Called after one bounded input in a separate fresh child that reloaded the
// prior v5 post-action candidate. Writes only a second disposable v5 slot.
void writeV5AfterReloadedInputCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> originalV4,
    std::span<const std::uint8_t> previousV5);

} // namespace mm6::testing
