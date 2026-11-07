#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"

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

// Indoor DLV actors use the same native 548-byte records as outdoor DDM
// actors. Read from the validated counted section, not from the MM7-shaped
// projection, which has no MM6 resistance owner.
inline std::vector<std::array<std::uint8_t, 6>> decodeNativeIndoorActorResistanceBytes(
    const MM6IndoorSnapshot &source, size_t expectedActorCount, GameTitle title) {
    if (title != GameTitle::MM6 || !source.isDelta)
        throw std::runtime_error("MM6 indoor actor resistance extraction requires an MM6 delta");
    const MM6IndoorSection &section = source.section(MM6IndoorSectionKind::Actors);
    if (section.count != expectedActorCount || section.bytes.size() < 4 ||
        expectedActorCount > (section.bytes.size() - 4) / 548 ||
        section.bytes.size() - 4 != expectedActorCount * 548)
        throw std::runtime_error("MM6 indoor actor resistance slots differ from decoded DLV");
    const auto *records = static_cast<const std::uint8_t *>(section.bytes.data());
    const std::uint32_t encodedCount = std::uint32_t(records[0]) |
                                       std::uint32_t(records[1]) << 8 |
                                       std::uint32_t(records[2]) << 16 |
                                       std::uint32_t(records[3]) << 24;
    if (encodedCount != expectedActorCount)
        throw std::runtime_error("MM6 indoor actor resistance count differs from DLV bytes");
    std::vector<std::array<std::uint8_t, 6>> result;
    result.reserve(expectedActorCount);
    for (size_t slot = 0; slot < expectedActorCount; ++slot) {
        const auto *actor = records + 4 + slot * 548;
        result.push_back({actor[80], actor[81], actor[82], actor[83], actor[84], actor[85]});
    }
    return result;
}

} // namespace mm6
