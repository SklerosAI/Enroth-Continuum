#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include "Engine/GameTitle.h"

namespace mm6 {

struct V5FullSessionPreflight;

// Build a detached, source-backed first-map v5 owner graph from candidate
// bytes and loaded MM6 resources. No live globals, save files or UI are
// changed; the caller must separately prove publication and post-world state.
std::unique_ptr<V5FullSessionPreflight> stageV5FirstMapLoadPreflight(
    std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
