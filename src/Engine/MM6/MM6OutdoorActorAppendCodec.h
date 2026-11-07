#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6OutdoorWire.h"

class Actor;

namespace mm6 {

constexpr size_t MM6_OUTDOOR_ACTOR_RECORD_BYTES = 548;

/**
 * Splice complete, caller-supplied native actor records after the original DDM
 * actor slots. Every source and appended slot must retain its live PID anchor.
 * This only constructs a detached wire; it does not capture native records from
 * runtime actors or authorize a live save/reload route.
 */
OutdoorDeltaWire appendMM6OutdoorActorRecords(
    const OutdoorDeltaWire &source, std::span<const Actor> finalActors,
    std::span<const std::uint8_t> appendedRecords, std::uint32_t lastRespawnDay,
    size_t maxActorSlots, GameTitle title);

} // namespace mm6
