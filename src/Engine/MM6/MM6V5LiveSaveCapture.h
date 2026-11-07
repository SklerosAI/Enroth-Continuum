#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace mm6 {

// First-map outdoor capture only. The caller provides a fully validated v4
// source whose native slots, ACTR and EVRT bindings are retained as anchors.
// Both APIs are read-only: they neither publish globals nor write a save slot.
// Refuse every owner the v5 schema cannot represent exactly.
std::vector<std::uint8_t> captureV5FirstMapLiveSession(
    std::span<const std::uint8_t> sourceV4);
void requireV5FirstMapLiveSessionAgreement(
    std::span<const std::uint8_t> candidateV5);

} // namespace mm6
