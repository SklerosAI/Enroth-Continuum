#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mm6::testing {

// Detached only: captures the settled second outdoor map without a slot write.
std::vector<std::uint8_t> captureV5LaterOutdoorForTest(
    std::span<const std::uint8_t> sourceV4);

// Called only from a disposable fresh child after exact oute2 v5 replay
// and one bounded input action. The prior v5 and original slots stay intact.
void writeV5LaterOutdoorAfterInputCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> originalV4,
    std::span<const std::uint8_t> previousV5);

} // namespace mm6::testing
