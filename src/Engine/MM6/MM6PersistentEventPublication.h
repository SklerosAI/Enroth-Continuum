#pragma once

#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"

#include "MM6PersistentEventBoundary.h"

namespace mm6 {

// Test-only fresh-child handoff before DoPrepareWorld(true). The existing
// detached stage already checked the selected DLV/DDM persistent section.
// Publish only the zero legacy owner; the proved 71 bytes stay in the map
// delta sidecar and are installed by the map loader.
void publishFreshChildPersistentEventZeros(
    PersistentVariables *destination,
    const DetachedPersistentEventBoundary &staged, GameTitle title);

// Read-only refusal gate after DoPrepareWorld(true) and onMapLoad(). The
// sidecar's final 256 bytes contain 200 persistent bytes and a 56-byte tail.
// A changed proved byte is conservatively rejected until event replay rules
// have a title-local transaction. This does not release active VISU slots.
void requirePostMapPersistentEvents(
    const DetachedPersistentEventBoundary &staged,
    std::span<const std::uint8_t> liveDecodedDelta,
    const PersistentVariables &liveLegacy, GameTitle title);

} // namespace mm6
