#include "MM6ActiveActorOutdoorDeltaCapture.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "MM6ChestRuntimeProjection.h"
#include "MM6ChestLiveOwnerAdapter.h"
#include "MM6OutdoorWire.h"

namespace mm6 {
namespace {

void put32(std::span<std::uint8_t> bytes, size_t offset, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes[offset + shift / 8] = static_cast<std::uint8_t>(value >> shift);
}

void requireExactSpriteProjection(const SpriteObject &source,
                                  const SpriteObject_MM7 &projected) {
    if (source.vPosition != projected.position.toFloat() ||
        source.vVelocity != projected.velocity.toFloat() ||
        source.initialPosition != projected.initialPosition.toFloat() ||
        source.uSectorID != projected.uSectorID ||
        source.timeSinceCreated.ticks() != projected.uTimeSinceCreated ||
        source.tempLifetime.ticks() != projected.tempLifetime ||
        source._lastParticleTime.ticks() != 0 || source._ticksPerParticle != 2_ticks)
        throw std::runtime_error("MM6 active-actor DDM sprite projection would lose runtime state");
}

} // namespace

Blob captureOutdoorDeltaWithActiveActors(const OutdoorLocation &location,
                                         GameTitle title) {
    if (title != GameTitle::MM6 || pOutdoor != &location ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR || location.mm6DeltaBytes.empty() ||
        location.ddm.respawnCount < 0 || location.ddm.lastRespawnDay <= 0)
        throw std::runtime_error("MM6 active-actor outdoor capture lacks loaded DDM state");

    OutdoorDeltaWire wire = decodeOutdoorDelta(location.mm6DeltaBytes);
    if (pActors.size() < wire.actorCount || pSpriteObjects.size() != wire.groundItemCount ||
        vChests.size() != wire.chestCount)
        throw std::runtime_error("MM6 active-actor outdoor capture has changed record counts");

    const auto actors = wire.part(OutdoorDeltaPart::Actors).subspan(4);
    for (size_t slot = 0; slot < pActors.size(); ++slot) {
        if (pActors[slot].id != slot)
            throw std::runtime_error("MM6 active-actor outdoor capture has discontinuous PID slots");
        if (slot >= wire.actorCount)
            continue;
        const Actor_MM7 native = projectMM6ActorRecord(actors.subspan(slot * 548, 548));
        const Actor &live = pActors[slot];
        if (native.monsterInfo.id != std::to_underlying(live.monsterInfo.id) ||
            native.monsterId != std::to_underlying(live.monsterId) ||
            native.initialPosition.toFloat() != live.initialPosition)
            throw std::runtime_error("MM6 active-actor outdoor capture has changed native actor identity");
    }

    auto reset = wire.part(OutdoorDeltaPart::ResetHeader);
    put32(reset, 0, location.ddm.respawnCount);
    put32(reset, 4, location.ddm.lastRespawnDay);
    auto visibility = wire.part(OutdoorDeltaPart::Visibility);
    std::memcpy(visibility.data(), location.uFullyRevealedCellOnMap.data(), 88 * 11);
    std::memcpy(visibility.data() + 88 * 11,
                location.uPartiallyRevealedCellOnMap.data(), 88 * 11);

    auto ground = wire.part(OutdoorDeltaPart::GroundItems).subspan(4);
    for (size_t slot = 0; slot < pSpriteObjects.size(); ++slot) {
        const SpriteObject &source = pSpriteObjects[slot];
        auto record = ground.subspan(slot * 100, 100);
        const SpriteObject_MM7 native = projectMM6GroundItemRecord(record);
        SpriteObject_MM7 live{};
        snapshot(source, &live);
        requireExactSpriteProjection(source, live);
        const auto *lhs = reinterpret_cast<const std::uint8_t *>(&live);
        const auto *rhs = reinterpret_cast<const std::uint8_t *>(&native);
        // MM6 item bytes 26..27 and the ground-item tail are retained opaque.
        // The current runtime does not own an exact MM6 item record here.
        if (std::memcmp(lhs + 36, rhs + 36, 26) ||
            std::memcmp(lhs + 72, rhs + 72, 36))
            throw std::runtime_error("MM6 active-actor outdoor capture has changed ground item data");
        std::memcpy(record.data(), lhs, 24);
        std::memcpy(record.data() + 26, lhs + 26, 10);
    }

    const auto chests = wire.part(OutdoorDeltaPart::Chests);
    ChestRecordSet records{};
    if (!copyChestRecords(chests, &records))
        throw std::runtime_error("MM6 active-actor outdoor capture lacks chest records");
    for (size_t slot = 0; slot < vChests.size(); ++slot) {
        const auto replay = stageChestRuntimeRecord(title,
            ChestRuntimeProjection{vChests[slot], records[slot]});
        if (!replay || *replay != records[slot])
            throw std::runtime_error("MM6 active-actor outdoor capture has changed chest data");
    }

    std::vector<std::uint8_t> encoded = encodeOutdoorDelta(wire);
    (void) decodeMM6OutdoorDelta(Blob::view(encoded.data(), encoded.size()));
    return Blob::copy(encoded.data(), encoded.size());
}

} // namespace mm6
