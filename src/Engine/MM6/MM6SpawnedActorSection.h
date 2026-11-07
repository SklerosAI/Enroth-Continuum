#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

class Actor;

namespace mm6 {

// Detached EC6-only candidate. The byte stream is not a native MM6 DDM actor
// record and is not part of the production raw-save manifest.
std::vector<std::uint8_t> encodeSpawnedActorSection(
    std::span<const Actor> actors, size_t firstSlot, GameTitle title);

// Exact slots are assigned only in the returned vector. No world is mutated.
std::vector<Actor> decodeSpawnedActorSection(
    std::span<const std::uint8_t> bytes, size_t expectedFirstSlot, GameTitle title);

} // namespace mm6
