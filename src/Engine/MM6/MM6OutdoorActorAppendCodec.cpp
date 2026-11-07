#include "MM6OutdoorActorAppendCodec.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Engine/Objects/Actor.h"

namespace mm6 {
namespace {

constexpr size_t MAX_DECODED_SIZE = 64 * 1024 * 1024;
constexpr size_t MAX_WIRE_ACTORS = 100000;

std::uint16_t read16(std::span<const std::uint8_t> bytes, size_t offset) {
    return std::uint16_t(bytes[offset]) | (std::uint16_t(bytes[offset + 1]) << 8);
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, size_t offset) {
    return std::uint32_t(read16(bytes, offset)) |
           (std::uint32_t(read16(bytes, offset + 2)) << 16);
}

void write32(std::span<std::uint8_t> bytes, size_t offset, std::uint32_t value) {
    for (size_t i = 0; i < 4; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
}

bool sameSlotAnchor(std::span<const std::uint8_t> record, const Actor &actor, size_t slot) {
    // These offsets are the MM6 projection's embedded monster ID (52),
    // monster ID (118), and initial position (146..151). NPC linkage can
    // legitimately change when the map generates its NPC pool.
    return actor.id == slot &&
           record[52] == std::to_underlying(actor.monsterInfo.id) &&
           static_cast<std::int16_t>(read16(record, 118)) == std::to_underlying(actor.monsterId) &&
           static_cast<std::int16_t>(read16(record, 146)) == actor.initialPosition.x &&
           static_cast<std::int16_t>(read16(record, 148)) == actor.initialPosition.y &&
           static_cast<std::int16_t>(read16(record, 150)) == actor.initialPosition.z;
}

} // namespace

OutdoorDeltaWire appendMM6OutdoorActorRecords(
    const OutdoorDeltaWire &source, std::span<const Actor> finalActors,
    std::span<const std::uint8_t> appendedRecords, std::uint32_t lastRespawnDay,
    size_t maxActorSlots, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 actor append requires MM6 title");
    // Decode afresh: callers cannot use stale/mutated part spans to choose a
    // splice point, and malformed source bytes never reach the encoder.
    const OutdoorDeltaWire original = decodeOutdoorDelta(source.bytes);
    const auto reset = original.part(OutdoorDeltaPart::ResetHeader);
    // OutdoorLocation::Load may already stamp the retained sidecar with the
    // current day before the initial encounter spawn pass runs.
    const std::uint32_t sourceDay = read32(reset, 4);
    if (read32(reset, 0) > std::numeric_limits<int>::max() ||
        (sourceDay != 0 && sourceDay != lastRespawnDay) ||
        lastRespawnDay == 0 || lastRespawnDay > std::numeric_limits<int>::max())
        throw std::runtime_error("MM6 actor append requires a matching respawn day");
    if (maxActorSlots == 0 || maxActorSlots > MAX_WIRE_ACTORS ||
        finalActors.size() > maxActorSlots || finalActors.size() <= original.actorCount)
        throw std::runtime_error("MM6 actor append exceeds slots or has no appended actors");
    const size_t appendCount = finalActors.size() - original.actorCount;
    if (appendCount > MAX_WIRE_ACTORS - original.actorCount ||
        appendCount > MAX_DECODED_SIZE / MM6_OUTDOOR_ACTOR_RECORD_BYTES ||
        appendedRecords.size() != appendCount * MM6_OUTDOOR_ACTOR_RECORD_BYTES ||
        appendedRecords.size() > MAX_DECODED_SIZE - original.bytes.size())
        throw std::runtime_error("MM6 appended actor record count exceeds bounds");

    const WireSpan actorPart = original.parts[static_cast<size_t>(OutdoorDeltaPart::Actors)];
    const auto originalRecords = original.part(OutdoorDeltaPart::Actors).subspan(4);
    for (size_t slot = 0; slot < finalActors.size(); ++slot) {
        const auto record = slot < original.actorCount
            ? originalRecords.subspan(slot * MM6_OUTDOOR_ACTOR_RECORD_BYTES, MM6_OUTDOOR_ACTOR_RECORD_BYTES)
            : appendedRecords.subspan((slot - original.actorCount) * MM6_OUTDOOR_ACTOR_RECORD_BYTES,
                                      MM6_OUTDOOR_ACTOR_RECORD_BYTES);
        if (!sameSlotAnchor(record, finalActors[slot], slot))
            throw std::runtime_error("MM6 actor PID slot was reused or native record anchor differs");
    }

    const size_t suffixOffset = actorPart.offset + actorPart.size;
    std::vector<std::uint8_t> bytes;
    bytes.reserve(original.bytes.size() + appendedRecords.size());
    bytes.insert(bytes.end(), original.bytes.begin(), original.bytes.begin() + suffixOffset);
    bytes.insert(bytes.end(), appendedRecords.begin(), appendedRecords.end());
    bytes.insert(bytes.end(), original.bytes.begin() + suffixOffset, original.bytes.end());
    write32(std::span(bytes).subspan(actorPart.offset, 4), 0,
            static_cast<std::uint32_t>(finalActors.size()));
    write32(std::span(bytes).subspan(4, 4), 0, lastRespawnDay);

    OutdoorDeltaWire result = decodeOutdoorDelta(bytes);
    if (result.actorCount != finalActors.size() ||
        !std::equal(original.bytes.begin() + suffixOffset, original.bytes.end(),
                    result.bytes.begin() + suffixOffset + appendedRecords.size()))
        throw std::runtime_error("MM6 actor append failed structural revalidation");
    return result;
}

} // namespace mm6
