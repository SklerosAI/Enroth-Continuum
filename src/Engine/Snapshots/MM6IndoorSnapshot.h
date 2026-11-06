#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Utility/Memory/Blob.h"

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
