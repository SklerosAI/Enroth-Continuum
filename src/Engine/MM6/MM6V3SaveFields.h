#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6GameplayRngSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6RawSaveFields.h"

namespace mm6 {

// Detached EC6 v3 manifest candidate. ACTR owns the complete active-map actor
// list in PID order, including native and spawned actors. A later map stage
// must bind that list to the selected active delta. No game state or save slot
// is published here.
struct V3SaveFields {
    RawSaveFields raw;
    std::vector<std::uint8_t> activeActorPayload;
    GeneratedNpcSaveFields generatedNpcs;
    GameplayRandomState gameplayRng;
};

// Exact 22-section container: the v2 raw owners plus ACTR, GNPC and RNGS.
std::vector<std::uint8_t> encodeV3Save(const V3SaveFields &fields, GameTitle title);
V3SaveFields decodeV3Save(std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
