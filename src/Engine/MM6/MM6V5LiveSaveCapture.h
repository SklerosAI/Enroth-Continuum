#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mm6 {

struct V5FullSessionPreflight;

enum class V5FirstMapCaptureBoundary {
    SettledInput,
    PausedSaveMenu,
};

// First-map outdoor capture only. The caller provides a fully validated v4
// source whose native slots, ACTR and EVRT bindings are retained as anchors.
// Both APIs are read-only: they neither publish globals nor write a save slot.
// Refuse every owner the v5 schema cannot represent exactly.
std::vector<std::uint8_t> captureV5FirstMapLiveSession(
    std::span<const std::uint8_t> sourceV4,
    V5FirstMapCaptureBoundary boundary = V5FirstMapCaptureBoundary::SettledInput);
void requireV5FirstMapLiveSessionAgreement(
    std::span<const std::uint8_t> candidateV5,
    V5FirstMapCaptureBoundary boundary = V5FirstMapCaptureBoundary::SettledInput);

// Read-only post-world check for a staged first-map v5 load. The caller still
// owns pre-event publication, failure handling and input admission. This
// function never opens a production MM6 save or changes the live world.
void requireV5FirstMapPostWorldAgreement(
    const V5FullSessionPreflight &staged, std::size_t expectedNativeActors);

} // namespace mm6
