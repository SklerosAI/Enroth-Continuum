#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6OutdoorWire.h"

namespace mm6 {

// Native MM6 creature bytes 80..85 are Fire, Electrical, Cold, Poison,
// Physical and Magical resistance in actor-local order. The table row is not
// a substitute: native DDM actors may carry individual overrides.
inline std::vector<std::array<std::uint8_t, 6>> decodeNativeActorResistanceBytes(
    const OutdoorDeltaWire &source, size_t expectedActorCount, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 actor resistance extraction requires MM6 title");
    const OutdoorDeltaWire checked = decodeOutdoorDelta(source.bytes);
    if (checked.actorCount != expectedActorCount || source.actorCount != checked.actorCount)
        throw std::runtime_error("MM6 actor resistance slots differ from decoded DDM");
    const auto records = checked.part(OutdoorDeltaPart::Actors).subspan(4);
    std::vector<std::array<std::uint8_t, 6>> result;
    result.reserve(checked.actorCount);
    for (size_t slot = 0; slot < checked.actorCount; ++slot) {
        const auto actor = records.subspan(slot * 548, 548);
        result.push_back({actor[80], actor[81], actor[82], actor[83], actor[84], actor[85]});
    }
    return result;
}

} // namespace mm6
