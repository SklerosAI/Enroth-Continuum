#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/Snapshots/EntitySnapshots.h"

class Blob;

// A runtime projection of a decoded MM6 DDM. The wire copy remains authoritative
// for every field without a proven MM6-to-engine mapping.
struct MM6OutdoorDeltaSnapshot {
    mm6::OutdoorDeltaWire wire;
    std::uint32_t respawnCount = 0;
    std::uint32_t lastRespawnDay = 0;
    std::array<std::array<std::uint8_t, 11>, 88> fullyRevealedCells{};
    std::array<std::array<std::uint8_t, 11>, 88> partiallyRevealedCells{};
    std::vector<Actor_MM7> actors;
    std::vector<SpriteObject_MM7> spriteObjects;
    std::vector<Chest_MM7> chests;
};

MM6OutdoorDeltaSnapshot decodeMM6OutdoorDelta(const Blob &source);

// Shared bounded MM6 record projections for outdoor and indoor deltas.
// These do not encode a changed record; callers must retain their wire bytes.
Actor_MM7 projectMM6ActorRecord(std::span<const std::uint8_t> record);
SpriteObject_MM7 projectMM6GroundItemRecord(std::span<const std::uint8_t> record);
Chest_MM7 projectMM6ChestRecord(std::span<const std::uint8_t> record);
