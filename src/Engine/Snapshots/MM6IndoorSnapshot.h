#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string_view>
#include <vector>

#include "EntitySnapshots.h"
#include "Utility/Memory/Blob.h"

struct IndoorLocation;
struct IndoorLocation_MM7;

// A bounded, lossless view of the MM6 BLV/DLV wire sections. Record field
// interpretation and runtime reconstruction are separate F4 work.
enum class MM6IndoorSectionKind {
    Header,
    Vertices,
    Faces,
    FaceData,
    FaceTextures,
    FaceExtras,
    FaceExtraTextures,
    Rooms,
    RoomData,
    RoomLightData,
    DoorSlots,
    Sprites,
    Lights,
    BspNodes,
    SpawnPoints,
    Outlines,
    ResetHeader,
    Visibility,
    Actors,
    GroundItems,
    Chests,
    DoorRecords,
    DoorData,
    PersistentBytes,
    Tail,
};

struct MM6IndoorSection {
    MM6IndoorSectionKind kind;
    Blob bytes;
    uint32_t count = 0; // Only set for counted sections.
};

struct MM6IndoorSnapshot {
    bool isDelta = false;
    uint32_t doorDataBytes = 0;
    std::vector<MM6IndoorSection> sections;

    const MM6IndoorSection &section(MM6IndoorSectionKind kind) const;
    void replaceSection(MM6IndoorSectionKind kind, const Blob &bytes);
};

MM6IndoorSnapshot deserializeMM6IndoorBase(const Blob &decoded);
MM6IndoorSnapshot deserializeMM6IndoorDelta(const Blob &decoded, const MM6IndoorSnapshot &base);
Blob serializeMM6Indoor(const MM6IndoorSnapshot &snapshot);
Blob decodeMM6IndoorBaseArchiveEntry(std::string_view name, const Blob &entry);

// The MM7-shaped value is an intermediate for the existing indoor renderer,
// never a substitute for the lossless MM6 wire snapshot kept by the caller.
IndoorLocation_MM7 projectMM6IndoorBase(const MM6IndoorSnapshot &base);
void reconstructMM6IndoorBase(const MM6IndoorSnapshot &base, IndoorLocation *dst);

struct MM6IndoorRuntimeDelta {
    MM6IndoorSnapshot wire;
    uint32_t respawnCount = 0;
    uint32_t lastRespawnDay = 0;
    std::array<char, 875> visibleOutlines{};
    std::vector<Actor_MM7> actors;
    std::vector<SpriteObject_MM7> spriteObjects;
    std::vector<Chest_MM7> chests;
    std::vector<BLVDoor_MM7> doors;
    std::vector<int16_t> doorsData;
};

MM6IndoorRuntimeDelta decodeMM6IndoorRuntimeDelta(const Blob &decoded, const MM6IndoorSnapshot &base);
void reconstructMM6IndoorDelta(const MM6IndoorRuntimeDelta &delta, IndoorLocation *dst);

// Patches only fields whose MM6 wire positions are established. The writer
// rejects unsupported edits and keeps every untouched byte from delta.wire.
Blob encodeMM6IndoorRuntimeDelta(const MM6IndoorRuntimeDelta &delta, const MM6IndoorSnapshot &base);

// Captures the supported fields from the active indoor runtime. The current
// sidecar supplies opaque bytes, including any event-variable edits.
Blob encodeMM6IndoorChangedState(const IndoorLocation &location);

/**
 * Like `encodeMM6IndoorChangedState`, but leaves the actor and ground item records as they were loaded, so the live
 * lists may have grown. Reset counters, visibility, chests and doors are the live ones.
 *
 * @param location                      The active indoor map.
 * @return                              The decoded DLV.
 */
Blob encodeMM6IndoorParkedState(const IndoorLocation &location);
