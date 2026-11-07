#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

class Actor;

namespace mm6 {

// ACTR v1 owns the complete active-map runtime actor list in exact PID order.
// It remains a detached EC6 candidate, outside live save/load publication.
std::vector<std::uint8_t> encodeActiveActorSection(
    std::span<const Actor> actors, GameTitle title);

std::vector<Actor> decodeActiveActorSection(
    std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
