#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6GameplayRngSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6RawSaveFields.h"

namespace mm6 {

// Detached EC6 v3 manifest candidate. The SPAW payload is the independently
// validated engine-native actor section; a later map stage must bind its first
// slot to the native DDM actor count. This type does not publish game state or
// write a save slot.
struct V3SaveFields {
    RawSaveFields raw;
    std::vector<std::uint8_t> spawnedActorPayload;
    GeneratedNpcSaveFields generatedNpcs;
    GameplayRandomState gameplayRng;
};

// Exact 22-section container: the v2 raw owners plus SPAW, GNPC and RNGS.
std::vector<std::uint8_t> encodeV3Save(const V3SaveFields &fields, GameTitle title);
V3SaveFields decodeV3Save(std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
