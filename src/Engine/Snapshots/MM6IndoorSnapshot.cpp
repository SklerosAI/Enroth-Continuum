#include "MM6IndoorSnapshot.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "CompositeSnapshots.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/MM6/MM6NativeActorResistanceBytes.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "MM6OutdoorDeltaSnapshot.h"
#include "Library/Snapshots/CommonSnapshots.h"
#include "Library/Compression/Compression.h"
#include "Utility/Exception.h"
#include "Utility/Streams/BlobOutputStream.h"

namespace {

constexpr size_t kMaximumDecodedBytes = 64 * 1024 * 1024;
constexpr size_t kHeaderBytes = 136;
constexpr uint32_t kDoorSlots = 200;

class Cursor {
 public:
    Cursor(const Blob &source, MM6IndoorSnapshot *snapshot) : _source(source), _snapshot(snapshot) {}

    uint32_t u32(size_t offset) const {
        if (offset > _source.size() || _source.size() - offset < 4)
            throw Exception("MM6 indoor file is missing a 32-bit field at {}", offset);
        const auto *p = static_cast<const uint8_t *>(_source.data()) + offset;
        return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
    }

    int16_t s16(size_t offset) const {
        if (offset > _source.size() || _source.size() - offset < 2)
            throw Exception("MM6 indoor file is missing a 16-bit field at {}", offset);
        const auto *p = static_cast<const uint8_t *>(_source.data()) + offset;
        return static_cast<int16_t>(uint16_t(p[0]) | uint16_t(p[1]) << 8);
    }

    void take(MM6IndoorSectionKind kind, size_t size, uint32_t count = 0) {
        if (_offset > _source.size() || size > _source.size() - _offset)
            throw Exception("MM6 indoor section {} exceeds decoded file", static_cast<int>(kind));
        _snapshot->sections.push_back({kind, _source.subBlob(_offset, size), count});
        _offset += size;
    }

    uint32_t counted(MM6IndoorSectionKind kind, size_t width) {
        uint32_t count = u32(_offset);
        if (count > (_source.size() - _offset - 4) / width)
            throw Exception("MM6 indoor section {} count {} exceeds decoded file", static_cast<int>(kind), count);
        take(kind, 4 + size_t(count) * width, count);
        return count;
    }

    [[nodiscard]] size_t position() const { return _offset; }

 private:
    const Blob &_source;
    MM6IndoorSnapshot *_snapshot;
    size_t _offset = 0;
};

void validateSourceSize(const Blob &decoded) {
    if (decoded.size() == 0 || decoded.size() > kMaximumDecodedBytes)
        throw Exception("MM6 indoor decoded file size {} is outside bounds", decoded.size());
}

} // namespace

const MM6IndoorSection &MM6IndoorSnapshot::section(MM6IndoorSectionKind kind) const {
    auto it = std::find_if(sections.begin(), sections.end(), [=](const MM6IndoorSection &part) { return part.kind == kind; });
    if (it == sections.end())
        throw Exception("MM6 indoor section {} is absent", static_cast<int>(kind));
    return *it;
}

void MM6IndoorSnapshot::replaceSection(MM6IndoorSectionKind kind, const Blob &bytes) {
    auto it = std::find_if(sections.begin(), sections.end(), [=](const MM6IndoorSection &part) { return part.kind == kind; });
    if (it == sections.end() || bytes.size() != it->bytes.size())
        throw Exception("MM6 indoor replacement section {} has wrong size", static_cast<int>(kind));
    it->bytes = Blob::copy(bytes);
}

MM6IndoorSnapshot deserializeMM6IndoorBase(const Blob &decoded) {
    validateSourceSize(decoded);
    if (decoded.size() < kHeaderBytes)
        throw Exception("MM6 indoor base header is truncated");

    MM6IndoorSnapshot result;
    Blob source = Blob::copy(decoded);
    Cursor cursor(source, &result);
    uint32_t faceDataBytes = cursor.u32(104);
    uint32_t roomDataBytes = cursor.u32(108);
    uint32_t roomLightBytes = cursor.u32(112);
    result.doorDataBytes = cursor.u32(116);

    cursor.take(MM6IndoorSectionKind::Header, kHeaderBytes);
    cursor.counted(MM6IndoorSectionKind::Vertices, 6);
    uint32_t faceCount = cursor.counted(MM6IndoorSectionKind::Faces, 80);
    cursor.take(MM6IndoorSectionKind::FaceData, faceDataBytes);
    cursor.take(MM6IndoorSectionKind::FaceTextures, size_t(faceCount) * 10);
    uint32_t extraCount = cursor.counted(MM6IndoorSectionKind::FaceExtras, 36);
    cursor.take(MM6IndoorSectionKind::FaceExtraTextures, size_t(extraCount) * 10);
    cursor.counted(MM6IndoorSectionKind::Rooms, 116);
    cursor.take(MM6IndoorSectionKind::RoomData, roomDataBytes);
    cursor.take(MM6IndoorSectionKind::RoomLightData, roomLightBytes);
    uint32_t doorCount = cursor.u32(cursor.position());
    cursor.take(MM6IndoorSectionKind::DoorSlots, 4, doorCount);
    if (doorCount != kDoorSlots)
        throw Exception("MM6 indoor base declares {} door slots, expected {}", doorCount, kDoorSlots);
    cursor.counted(MM6IndoorSectionKind::Sprites, 60);
    cursor.counted(MM6IndoorSectionKind::Lights, 12);
    cursor.counted(MM6IndoorSectionKind::BspNodes, 8);
    cursor.counted(MM6IndoorSectionKind::SpawnPoints, 20);
    cursor.counted(MM6IndoorSectionKind::Outlines, 12);
    if (cursor.position() != source.size())
        throw Exception("MM6 indoor base has {} trailing bytes", source.size() - cursor.position());
    return result;
}

MM6IndoorSnapshot deserializeMM6IndoorDelta(const Blob &decoded, const MM6IndoorSnapshot &base) {
    validateSourceSize(decoded);
    if (base.isDelta || base.section(MM6IndoorSectionKind::DoorSlots).count != kDoorSlots)
        throw Exception("MM6 indoor delta requires a validated base map");

    MM6IndoorSnapshot result;
    result.isDelta = true;
    result.doorDataBytes = base.doorDataBytes;
    Blob source = Blob::copy(decoded);
    Cursor cursor(source, &result);
    cursor.take(MM6IndoorSectionKind::ResetHeader, 8);
    cursor.take(MM6IndoorSectionKind::Visibility, 875);
    cursor.counted(MM6IndoorSectionKind::Actors, 548);
    cursor.counted(MM6IndoorSectionKind::GroundItems, 100);
    cursor.counted(MM6IndoorSectionKind::Chests, 4204);

    size_t doorOffset = cursor.position();
    cursor.take(MM6IndoorSectionKind::DoorRecords, kDoorSlots * 80);
    size_t computedDoorBytes = 0;
    for (size_t i = 0; i < kDoorSlots; ++i) {
        size_t countsOffset = doorOffset + i * 80 + 68;
        int16_t vertices = cursor.s16(countsOffset);
        int16_t faces = cursor.s16(countsOffset + 2);
        int16_t rooms = cursor.s16(countsOffset + 4);
        int16_t offsets = cursor.s16(countsOffset + 6);
        if (vertices < 0 || faces < 0 || rooms < 0 || offsets < 0)
            throw Exception("MM6 indoor door {} has negative array count", i);
        computedDoorBytes += 2 * (size_t(vertices) + 3 * size_t(faces) + size_t(rooms) + 3 * size_t(offsets));
        if (computedDoorBytes > result.doorDataBytes)
            throw Exception("MM6 indoor door arrays exceed base map size");
    }
    if (computedDoorBytes != result.doorDataBytes)
        throw Exception("MM6 indoor door arrays do not match base map size");
    cursor.take(MM6IndoorSectionKind::DoorData, result.doorDataBytes);
    cursor.take(MM6IndoorSectionKind::PersistentBytes, 200);
    cursor.take(MM6IndoorSectionKind::Tail, 56);
    if (cursor.position() != source.size())
        throw Exception("MM6 indoor delta has {} trailing bytes", source.size() - cursor.position());
    return result;
}

Blob serializeMM6Indoor(const MM6IndoorSnapshot &snapshot) {
    Blob output;
    BlobOutputStream stream(&output);
    for (const MM6IndoorSection &part : snapshot.sections)
        stream.write(part.bytes);
    stream.close();

    // A same-width opaque replacement is only valid if the whole wire layout
    // remains bounded. Keep that invariant even before field semantics are known.
    if (snapshot.isDelta) {
        MM6IndoorSnapshot base;
        base.doorDataBytes = snapshot.doorDataBytes;
        base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), kDoorSlots});
        deserializeMM6IndoorDelta(output, base);
    } else {
        deserializeMM6IndoorBase(output);
    }
    return output;
}

Blob decodeMM6IndoorBaseArchiveEntry(std::string_view name, const Blob &entry) {
    try {
        return decodeMM6ArchiveEntry(entry);
    } catch (const Exception &) {
        // Only these two original-matched MM6 BLV entries have an invalid
        // checksum trailer. Their raw DEFLATE stream and decoded length were
        // checked independently in F3. Never extend this to arbitrary maps.
        if (name != "cd3.blv" && name != "d08.blv")
            throw;
        if (entry.size() < 14)
            throw;
        const auto *header = static_cast<const uint8_t *>(entry.data());
        uint32_t compressed = uint32_t(header[0]) | uint32_t(header[1]) << 8 |
                              uint32_t(header[2]) << 16 | uint32_t(header[3]) << 24;
        uint32_t expected = uint32_t(header[4]) | uint32_t(header[5]) << 8 |
                            uint32_t(header[6]) << 16 | uint32_t(header[7]) << 24;
        if (compressed != entry.size() - 8 || expected == 0 || expected > kMaximumDecodedBytes)
            throw;
        Blob decoded = zlib::uncompressBestEffort(entry.subBlob(8, compressed), expected);
        if (decoded.size() != expected)
            throw Exception("MM6 indoor base '{}' recovered length differs from header", name);
        (void) deserializeMM6IndoorBase(decoded);
        return decoded;
    }
}

namespace {

template<class T>
std::vector<T> records(const MM6IndoorSection &section, size_t wireWidth, bool counted = true) {
    static_assert(std::is_trivially_copyable_v<T>);
    size_t prefix = counted ? 4 : 0;
    size_t count = counted ? section.count : section.bytes.size() / wireWidth;
    if (section.bytes.size() != prefix + count * wireWidth || sizeof(T) != wireWidth)
        throw Exception("MM6 indoor section {} has an invalid record width", static_cast<int>(section.kind));
    std::vector<T> result(count);
    if (count)
        std::memcpy(result.data(), static_cast<const uint8_t *>(section.bytes.data()) + prefix, count * wireWidth);
    return result;
}

template<class T>
std::vector<T> fixedWords(const MM6IndoorSection &section) {
    return records<T>(section, sizeof(T), false);
}

struct MM6Decoration {
    uint16_t id;
    uint16_t flags;
    Vec3i position;
    int32_t yaw;
    int16_t eventVariable;
    int16_t eventId;
    int16_t triggerRadius;
    int16_t direction;
};
static_assert(sizeof(MM6Decoration) == 28);

} // namespace

IndoorLocation_MM7 projectMM6IndoorBase(const MM6IndoorSnapshot &base) {
    if (base.isDelta)
        throw Exception("MM6 indoor base projection received a delta");

    IndoorLocation_MM7 result{};
    result.vertices = records<Vec3s>(base.section(MM6IndoorSectionKind::Vertices), 6);

    const MM6IndoorSection &faces = base.section(MM6IndoorSectionKind::Faces);
    result.faces.resize(faces.count);
    if (faces.bytes.size() != 4 + faces.count * 80)
        throw Exception("MM6 indoor face records have invalid length");
    const auto *faceBytes = static_cast<const uint8_t *>(faces.bytes.data()) + 4;
    for (size_t i = 0; i < result.faces.size(); ++i) {
        const uint8_t *wire = faceBytes + i * 80;
        BLVFace_MM7 &face = result.faces[i];
        // MM6's 80-byte face is the MM7 record without its leading 16-byte
        // floating-point plane. Its fixed-point plane remains byte-exact.
        std::memcpy(reinterpret_cast<uint8_t *>(&face) + 16, wire, 80);
        face.facePlane.normal = face.facePlaneOld.normal.toFloatFromFixpoint();
        face.facePlane.dist = static_cast<float>(face.facePlaneOld.dist) / 65536.0f;
        if (face.faceExtraId >= base.section(MM6IndoorSectionKind::FaceExtras).count)
            throw Exception("MM6 indoor face {} references absent extra {}", i, face.faceExtraId);
    }

    result.faceData = fixedWords<int16_t>(base.section(MM6IndoorSectionKind::FaceData));
    size_t faceDataIndex = 0;
    for (size_t i = 0; i < result.faces.size(); ++i) {
        size_t vertices = result.faces[i].numVertices;
        size_t words = 6 * (vertices + 1);
        if (words > result.faceData.size() - faceDataIndex)
            throw Exception("MM6 indoor face {} exceeds its variable data", i);
        for (size_t j = 0; j < vertices; ++j)
            if (result.faceData[faceDataIndex + j] < 0 ||
                size_t(result.faceData[faceDataIndex + j]) >= result.vertices.size())
                throw Exception("MM6 indoor face {} references an absent vertex", i);
        faceDataIndex += words;
    }
    if (faceDataIndex != result.faceData.size())
        throw Exception("MM6 indoor face data has unused words");
    result.faceTextures = fixedWords<std::array<char, 10>>(base.section(MM6IndoorSectionKind::FaceTextures));
    result.faceExtras = records<BLVFaceExtra_MM7>(base.section(MM6IndoorSectionKind::FaceExtras), 36);
    result.faceExtraTextures = fixedWords<std::array<char, 10>>(base.section(MM6IndoorSectionKind::FaceExtraTextures));
    result.sectors = records<BLVSector_MM7>(base.section(MM6IndoorSectionKind::Rooms), 116);
    result.sectorData = fixedWords<uint16_t>(base.section(MM6IndoorSectionKind::RoomData));
    result.sectorLightData = fixedWords<uint16_t>(base.section(MM6IndoorSectionKind::RoomLightData));
    result.doorCount = base.section(MM6IndoorSectionKind::DoorSlots).count;

    const MM6IndoorSection &sprites = base.section(MM6IndoorSectionKind::Sprites);
    if (sprites.bytes.size() != 4 + sprites.count * 60)
        throw Exception("MM6 indoor decoration records have invalid length");
    result.decorations.resize(sprites.count);
    result.decorationNames.resize(sprites.count);
    const auto *spriteBytes = static_cast<const uint8_t *>(sprites.bytes.data()) + 4;
    const auto *spriteNames = spriteBytes + sprites.count * sizeof(MM6Decoration); // All records come first, then all names.
    for (size_t i = 0; i < sprites.count; ++i) {
        MM6Decoration source;
        std::memcpy(&source, spriteBytes + i * sizeof(MM6Decoration), sizeof(source));
        LevelDecoration_MM7 &target = result.decorations[i];
        target.uDecorationDescID = source.id;
        target.uFlags = source.flags;
        target.vPosition = source.position;
        target._yawAngle = source.yaw;
        target.uEventID = source.eventId;
        target.uTriggerRange = source.triggerRadius;
        target.eventVarId = source.eventVariable;
        target.field_1A = source.direction;
        std::memcpy(result.decorationNames[i].data(), spriteNames + i * 32, 32);
    }

    const std::vector<BLVLight_MM6> lights = records<BLVLight_MM6>(base.section(MM6IndoorSectionKind::Lights), 12);
    result.lights.reserve(lights.size());
    for (const BLVLight_MM6 &source : lights) {
        BLVLight_MM7 &target = result.lights.emplace_back();
        target.vPosition = source.vPosition;
        target.uRadius = source.uRadius;
        target.uRed = target.uGreen = target.uBlue = static_cast<char>(255);
        target.uType = 0;
        target.uAtributes = source.uAttributes;
        target.uBrightness = source.uBrightness;
    }
    result.bspNodes = records<BSPNode_MM7>(base.section(MM6IndoorSectionKind::BspNodes), 8);
    const std::vector<SpawnPoint_MM6> spawns = records<SpawnPoint_MM6>(base.section(MM6IndoorSectionKind::SpawnPoints), 20);
    result.spawnPoints.reserve(spawns.size());
    for (const SpawnPoint_MM6 &source : spawns) {
        SpawnPoint_MM7 &target = result.spawnPoints.emplace_back();
        target.position = source.position;
        target.radius = source.radius;
        target.type = source.type;
        target.treasureLevelOrMonsterIndex = source.treasureLevelOrMonsterIndex;
        target.attributes = source.attributes;
    }
    result.mapOutlines = records<BLVMapOutline_MM7>(base.section(MM6IndoorSectionKind::Outlines), 12);
    size_t sectorWord = 0;
    size_t lightWord = 0;
    for (size_t i = 0; i < result.sectors.size(); ++i) {
        BLVSector_MM7 &sector = result.sectors[i];
        // A few shipped no-BSP rooms encode zero where the runtime uses -1.
        // Keep the original word in the base wire snapshot.
        if (result.bspNodes.empty() && sector.firstBspNode == 0)
            sector.firstBspNode = -1;
        if (sector.numPortals < 0 || sector.numNonBspFaces > sector.numFaces)
            throw Exception("MM6 indoor sector {} has invalid face counts", i);
        const size_t faceGroups[] = {sector.numFloors, sector.numWalls, sector.numCeilings,
                                     sector.numFluids, size_t(sector.numPortals), sector.numFaces};
        for (size_t count : faceGroups) {
            if (count > result.sectorData.size() - sectorWord)
                throw Exception("MM6 indoor sector {} exceeds room data", i);
            for (size_t j = 0; j < count; ++j)
                if (result.sectorData[sectorWord + j] >= result.faces.size())
                    throw Exception("MM6 indoor sector {} references an absent face", i);
            sectorWord += count;
        }
        const size_t otherGroups[] = {sector.numCogs, sector.numDecorations, sector.numMarkers};
        for (size_t group = 0; group < 3; ++group) {
            size_t count = otherGroups[group];
            if (count > result.sectorData.size() - sectorWord)
                throw Exception("MM6 indoor sector {} exceeds room data", i);
            if (group == 1) {
                for (size_t j = 0; j < count; ++j)
                    if (result.sectorData[sectorWord + j] >= result.decorations.size())
                        throw Exception("MM6 indoor sector {} references an absent decoration", i);
            }
            sectorWord += count;
        }
        if (sector.numLights > result.sectorLightData.size() - lightWord)
            throw Exception("MM6 indoor sector {} exceeds room light data", i);
        for (size_t j = 0; j < sector.numLights; ++j)
            if (result.sectorLightData[lightWord + j] >= result.lights.size())
                throw Exception("MM6 indoor sector {} references an absent light", i);
        lightWord += sector.numLights;
        if (sector.firstBspNode < -1 ||
            (sector.firstBspNode >= 0 && size_t(sector.firstBspNode) >= result.bspNodes.size()))
            throw Exception("MM6 indoor sector {} references an absent BSP node", i);
    }
    if (sectorWord != result.sectorData.size() || lightWord != result.sectorLightData.size())
        throw Exception("MM6 indoor room arrays have unused words");
    return result;
}

void reconstructMM6IndoorBase(const MM6IndoorSnapshot &base, IndoorLocation *dst) {
    IndoorLocation_MM7 projected = projectMM6IndoorBase(base);
    reconstruct(projected, dst);
    // MM6 decoration event variables are not MM7's 75-offset encoding.
    for (size_t i = 0; i < projected.decorations.size(); ++i)
        pLevelDecorations[i].eventVarId = projected.decorations[i].eventVarId;
}

namespace {

uint32_t word32(const uint8_t *p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

template<class T, class Projection>
std::vector<T> projectDeltaRecords(const MM6IndoorSection &part, size_t width, Projection project) {
    if (part.bytes.size() != 4 + size_t(part.count) * width)
        throw Exception("MM6 indoor delta section {} has invalid length", static_cast<int>(part.kind));
    std::vector<T> result;
    result.reserve(part.count);
    const auto *p = static_cast<const uint8_t *>(part.bytes.data()) + 4;
    for (size_t i = 0; i < part.count; ++i)
        result.push_back(project(std::span<const uint8_t>(p + i * width, width)));
    return result;
}

} // namespace

MM6IndoorRuntimeDelta decodeMM6IndoorRuntimeDelta(const Blob &decoded, const MM6IndoorSnapshot &base) {
    MM6IndoorRuntimeDelta result;
    result.wire = deserializeMM6IndoorDelta(decoded, base);
    const auto &reset = result.wire.section(MM6IndoorSectionKind::ResetHeader).bytes;
    const auto *resetBytes = static_cast<const uint8_t *>(reset.data());
    result.respawnCount = word32(resetBytes);
    result.lastRespawnDay = word32(resetBytes + 4);
    if (result.respawnCount > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        result.lastRespawnDay > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        throw Exception("MM6 indoor reset header exceeds runtime range");

    const auto &visibility = result.wire.section(MM6IndoorSectionKind::Visibility).bytes;
    std::memcpy(result.visibleOutlines.data(), visibility.data(), result.visibleOutlines.size());
    result.actors = projectDeltaRecords<Actor_MM7>(result.wire.section(MM6IndoorSectionKind::Actors), 548, projectMM6ActorRecord);
    result.spriteObjects = projectDeltaRecords<SpriteObject_MM7>(result.wire.section(MM6IndoorSectionKind::GroundItems), 100, projectMM6GroundItemRecord);
    result.chests = projectDeltaRecords<Chest_MM7>(result.wire.section(MM6IndoorSectionKind::Chests), 4204, projectMM6ChestRecord);
    result.doors = fixedWords<BLVDoor_MM7>(result.wire.section(MM6IndoorSectionKind::DoorRecords));
    result.doorsData = fixedWords<int16_t>(result.wire.section(MM6IndoorSectionKind::DoorData));
    size_t cursor = 0;
    for (size_t i = 0; i < result.doors.size(); ++i) {
        const BLVDoor_MM7 &door = result.doors[i];
        const size_t needed = size_t(door.numVertices) + 3 * size_t(door.numFaces) +
                              size_t(door.numSectors) + 3 * size_t(door.numOffsets);
        if (door.numOffsets < door.numVertices || cursor > result.doorsData.size() ||
            needed > result.doorsData.size() - cursor)
            throw Exception("MM6 indoor door {} has invalid variable data", i);
        for (size_t j = 0; j < door.numVertices; ++j)
            if (result.doorsData[cursor + j] < 0 ||
                size_t(result.doorsData[cursor + j]) >= base.section(MM6IndoorSectionKind::Vertices).count)
                throw Exception("MM6 indoor door {} references an absent vertex", i);
        cursor += door.numVertices;
        for (size_t j = 0; j < door.numFaces; ++j)
            if (result.doorsData[cursor + j] < 0 ||
                size_t(result.doorsData[cursor + j]) >= base.section(MM6IndoorSectionKind::Faces).count)
                throw Exception("MM6 indoor door {} references an absent face", i);
        cursor += door.numFaces;
        for (size_t j = 0; j < door.numSectors; ++j)
            if (result.doorsData[cursor + j] < 0 ||
                size_t(result.doorsData[cursor + j]) >= base.section(MM6IndoorSectionKind::Rooms).count)
                throw Exception("MM6 indoor door {} references an absent sector", i);
        cursor += 2 * size_t(door.numFaces) + size_t(door.numSectors) + 3 * size_t(door.numOffsets);
    }
    if (cursor != result.doorsData.size())
        throw Exception("MM6 indoor door data has unused words");
    return result;
}

void reconstructMM6IndoorDelta(const MM6IndoorRuntimeDelta &delta, IndoorLocation *dst) {
    dst->dlv.respawnCount = delta.respawnCount;
    dst->dlv.lastRespawnDay = delta.lastRespawnDay;
    dst->_visible_outlines = delta.visibleOutlines;
    if (dst->mapOutlines.size() > dst->_visible_outlines.size() * 8)
        throw Exception("MM6 indoor map has too many visibility outlines");
    for (size_t i = 0; i < dst->mapOutlines.size(); ++i) {
        if (static_cast<uint8_t>(dst->_visible_outlines[i / 8]) & (1u << (7 - i % 8)))
            dst->mapOutlines[i].uFlags |= 1;
    }

    const auto actorResistances = mm6::decodeNativeIndoorActorResistanceBytes(
        delta.wire, delta.actors.size(), GameTitle::MM6);
    reconstruct(delta.actors, &pActors);
    nextActorReuseScanStart = 0;
    for (size_t i = 0; i < pActors.size(); ++i) {
        pActors[i].id = i;
        const auto &res = actorResistances[i];
        pActors[i].monsterInfo.mm6Resistances = mm6::MonsterResistances{
            res[0], res[1], res[2], res[3], res[4], res[5]};
    }
    reconstruct(delta.spriteObjects, &pSpriteObjects);
    vChests.resize(delta.chests.size());
    for (size_t i = 0; i < delta.chests.size(); ++i)
        reconstruct(delta.chests[i], &vChests[i], tags::context<int>(i));

    reconstruct(delta.doors, &dst->doors);
    dst->doorsData = delta.doorsData;
    size_t cursor = 0;
    for (BLVDoor &door : dst->doors) {
        const size_t needed = size_t(door.numVertices) + 3 * size_t(door.numFaces) +
                              size_t(door.numSectors) + 3 * size_t(door.numOffsets);
        if (needed > dst->doorsData.size() - cursor)
            throw Exception("MM6 indoor door arrays exceed their wire section");
        door.pVertexIDs = dst->doorsData.data() + cursor;
        cursor += door.numVertices;
        door.pFaceIDs = dst->doorsData.data() + cursor;
        cursor += door.numFaces;
        door.pSectorIDs = dst->doorsData.data() + cursor;
        cursor += door.numSectors;
        door.pDeltaUs = dst->doorsData.data() + cursor;
        cursor += door.numFaces;
        door.pDeltaVs = dst->doorsData.data() + cursor;
        cursor += door.numFaces;
        size_t x = cursor;
        cursor += door.numOffsets;
        size_t y = cursor;
        cursor += door.numOffsets;
        size_t z = cursor;
        cursor += door.numOffsets;
        door.offsets.resize(door.numOffsets);
        for (size_t i = 0; i < door.numOffsets; ++i)
            door.offsets[i] = Vec3f(dst->doorsData[x + i], dst->doorsData[y + i], dst->doorsData[z + i]);
        for (size_t i = 0; i < door.numVertices; ++i)
            if (door.pVertexIDs[i] < 0 || size_t(door.pVertexIDs[i]) >= dst->vertices.size())
                throw Exception("MM6 indoor door references an absent vertex");
        for (size_t i = 0; i < door.numFaces; ++i) {
            if (door.pFaceIDs[i] < 0 || size_t(door.pFaceIDs[i]) >= dst->faces.size())
                throw Exception("MM6 indoor door references an absent face");
            const BLVFace &face = dst->faces[door.pFaceIDs[i]];
            door.pDeltaUs[i] = face.textureDeltaU;
            door.pDeltaVs[i] = face.textureDeltaV;
        }
        for (size_t i = 0; i < door.numSectors; ++i)
            if (door.pSectorIDs[i] < 0 || size_t(door.pSectorIDs[i]) >= dst->sectors.size())
                throw Exception("MM6 indoor door references an absent sector");
    }
    if (cursor != dst->doorsData.size())
        throw Exception("MM6 indoor door arrays leave unused bytes");

    // The 200 persistent bytes and 56-byte tail remain in delta.wire. Their
    // MM6 meanings are not established by the zero-filled shipped deltas.
}

namespace {

static_assert(std::endian::native == std::endian::little);
static_assert(offsetof(Actor_MM7, field_84) == 132);
static_assert(offsetof(SpriteObject_MM7, containing_item) == 36);
static_assert(offsetof(SpriteObject_MM7, uSpellID) == 72);

void copyBytes(void *dst, const void *src, size_t offset, size_t length) {
    std::memcpy(static_cast<uint8_t *>(dst) + offset, static_cast<const uint8_t *>(src) + offset, length);
}

void write32(char *dst, uint32_t value) {
    for (size_t i = 0; i < 4; ++i)
        dst[i] = static_cast<char>(value >> (8 * i));
}

void replaceRecords(MM6IndoorSnapshot *wire, MM6IndoorSectionKind kind, const std::string &bytes) {
    wire->replaceSection(kind, Blob::fromString(bytes));
}

} // namespace

Blob encodeMM6IndoorRuntimeDelta(const MM6IndoorRuntimeDelta &delta, const MM6IndoorSnapshot &base) {
    if (!delta.wire.isDelta)
        throw Exception("MM6 indoor writer requires a DLV wire snapshot");
    MM6IndoorRuntimeDelta original = decodeMM6IndoorRuntimeDelta(serializeMM6Indoor(delta.wire), base);
    if (delta.actors.size() != original.actors.size() ||
        delta.spriteObjects.size() != original.spriteObjects.size() ||
        delta.chests.size() != original.chests.size() ||
        delta.doors.size() != original.doors.size() ||
        delta.doorsData != original.doorsData)
        throw Exception("MM6 indoor writer cannot change record counts or door geometry arrays");
    if (delta.respawnCount > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        delta.lastRespawnDay > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        throw Exception("MM6 indoor reset counter exceeds runtime range");

    MM6IndoorSnapshot wire = std::move(original.wire);
    std::string reset(8, '\0');
    write32(reset.data(), delta.respawnCount);
    write32(reset.data() + 4, delta.lastRespawnDay);
    replaceRecords(&wire, MM6IndoorSectionKind::ResetHeader, reset);
    wire.replaceSection(MM6IndoorSectionKind::Visibility,
                        Blob::copy(delta.visibleOutlines.data(), delta.visibleOutlines.size()));

    std::string actors(static_cast<const char *>(wire.section(MM6IndoorSectionKind::Actors).bytes.data()),
                       wire.section(MM6IndoorSectionKind::Actors).bytes.size());
    for (size_t i = 0; i < delta.actors.size(); ++i) {
        const Actor_MM7 &current = delta.actors[i];
        Actor_MM7 permitted = original.actors[i];
        if (current.hp < 0)
            throw Exception("MM6 indoor actor {} has unverified negative HP encoding", i);
        permitted.attributes = current.attributes;
        permitted.hp = current.hp;
        // MM6's 118..165 and 168..195 map to the same runtime fields as
        // MM7's 134..181 and 184..211. Leave both padding words intact.
        copyBytes(&permitted, &current, 134, 48);
        copyBytes(&permitted, &current, 184, 28);
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw Exception("MM6 indoor actor {} has unsupported changed fields", i);
        char *record = actors.data() + 4 + i * 548;
        write32(record + 36, current.attributes);
        std::memcpy(record + 40, &current.hp, sizeof(current.hp));
        std::memcpy(record + 118, reinterpret_cast<const uint8_t *>(&current) + 134, 48);
        std::memcpy(record + 168, reinterpret_cast<const uint8_t *>(&current) + 184, 28);
    }
    replaceRecords(&wire, MM6IndoorSectionKind::Actors, actors);

    std::string ground(static_cast<const char *>(wire.section(MM6IndoorSectionKind::GroundItems).bytes.data()),
                       wire.section(MM6IndoorSectionKind::GroundItems).bytes.size());
    for (size_t i = 0; i < delta.spriteObjects.size(); ++i) {
        const SpriteObject_MM7 &current = delta.spriteObjects[i];
        SpriteObject_MM7 permitted = original.spriteObjects[i];
        copyBytes(&permitted, &current, 0, 24);
        copyBytes(&permitted, &current, 26, 38);
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw Exception("MM6 indoor ground item {} has unsupported changed fields", i);
        char *record = ground.data() + 4 + i * 100;
        std::memcpy(record, &current, 24);
        std::memcpy(record + 26, reinterpret_cast<const uint8_t *>(&current) + 26, 38);
    }
    replaceRecords(&wire, MM6IndoorSectionKind::GroundItems, ground);

    std::string chests(static_cast<const char *>(wire.section(MM6IndoorSectionKind::Chests).bytes.data()),
                       wire.section(MM6IndoorSectionKind::Chests).bytes.size());
    for (size_t i = 0; i < delta.chests.size(); ++i) {
        const Chest_MM7 &current = delta.chests[i];
        if (!mm6::validChestRuntimeGrid(GameTitle::MM6, current))
            throw Exception("MM6 indoor chest {} has invalid active grid", i);
        Chest_MM7 permitted = original.chests[i];
        permitted.flags = current.flags;
        permitted.inventoryMatrix = current.inventoryMatrix;
        for (size_t j = 0; j < current.items.size(); ++j)
            std::memcpy(&permitted.items[j], &current.items[j], 28);
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw Exception("MM6 indoor chest {} has unsupported type or item fields", i);
        char *record = chests.data() + 4 + i * 4204;
        std::memcpy(record + 2, &current.flags, sizeof(current.flags));
        for (size_t j = 0; j < current.items.size(); ++j)
            std::memcpy(record + 4 + j * 28, &current.items[j], 28);
        std::memcpy(record + 3924, current.inventoryMatrix.data(), 280);
    }
    replaceRecords(&wire, MM6IndoorSectionKind::Chests, chests);

    std::string doors(static_cast<const char *>(wire.section(MM6IndoorSectionKind::DoorRecords).bytes.data()),
                      wire.section(MM6IndoorSectionKind::DoorRecords).bytes.size());
    for (size_t i = 0; i < delta.doors.size(); ++i) {
        const BLVDoor_MM7 &current = delta.doors[i];
        BLVDoor_MM7 permitted = original.doors[i];
        permitted.attributes = current.attributes;
        permitted.timeSinceTriggered = current.timeSinceTriggered;
        permitted.state = current.state;
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw Exception("MM6 indoor door {} has unsupported changed geometry", i);
        char *record = doors.data() + i * 80;
        write32(record, current.attributes);
        write32(record + 8, current.timeSinceTriggered);
        std::memcpy(record + 76, &current.state, sizeof(current.state));
    }
    replaceRecords(&wire, MM6IndoorSectionKind::DoorRecords, doors);

    Blob encoded = serializeMM6Indoor(wire);
    (void) decodeMM6IndoorRuntimeDelta(encoded, base);
    return encoded;
}

Blob encodeMM6IndoorChangedState(const IndoorLocation &location) {
    if (location.mm6DeltaBytes.empty() || location.dlv.respawnCount < 0 || location.dlv.lastRespawnDay < 0 ||
        location.vertices.size() > std::numeric_limits<uint32_t>::max() ||
        location.faces.size() > std::numeric_limits<uint32_t>::max() ||
        location.sectors.size() > std::numeric_limits<uint32_t>::max() ||
        location.doorsData.size() > std::numeric_limits<uint32_t>::max() / 2)
        throw Exception("MM6 indoor runtime lacks bounded delta state");

    // The DLV parser needs only the base's door-array byte count and the
    // projected vertex/face/room counts. These were checked on BLV load.
    MM6IndoorSnapshot base;
    base.doorDataBytes = location.doorsData.size() * 2;
    base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
    base.sections.push_back({MM6IndoorSectionKind::Vertices, Blob(), static_cast<uint32_t>(location.vertices.size())});
    base.sections.push_back({MM6IndoorSectionKind::Faces, Blob(), static_cast<uint32_t>(location.faces.size())});
    base.sections.push_back({MM6IndoorSectionKind::Rooms, Blob(), static_cast<uint32_t>(location.sectors.size())});
    Blob retained = Blob::copy(location.mm6DeltaBytes.data(), location.mm6DeltaBytes.size());
    MM6IndoorRuntimeDelta current = decodeMM6IndoorRuntimeDelta(retained, base);
    current.respawnCount = location.dlv.respawnCount;
    current.lastRespawnDay = location.dlv.lastRespawnDay;
    current.visibleOutlines = location._visible_outlines;
    if (pActors.size() != current.actors.size() || pSpriteObjects.size() != current.spriteObjects.size() ||
        vChests.size() != current.chests.size() || location.doors.size() != current.doors.size() ||
        location.doorsData.size() != current.doorsData.size())
        throw Exception("MM6 indoor runtime record count changed; save encoding is not established");

    for (size_t i = 0; i < current.actors.size(); ++i) {
        Actor_MM7 live{};
        snapshot(pActors[i], &live);
        current.actors[i].attributes = live.attributes;
        current.actors[i].hp = live.hp;
        copyBytes(&current.actors[i], &live, 134, 48);
        copyBytes(&current.actors[i], &live, 184, 28);
    }
    for (size_t i = 0; i < current.spriteObjects.size(); ++i) {
        SpriteObject_MM7 live{};
        snapshot(pSpriteObjects[i], &live);
        copyBytes(&current.spriteObjects[i], &live, 0, 24);
        copyBytes(&current.spriteObjects[i], &live, 26, 38);
    }
    for (size_t i = 0; i < current.chests.size(); ++i) {
        Chest_MM7 live{};
        snapshot(vChests[i], &live);
        if (!mm6::validChestRuntimeGrid(GameTitle::MM6, live))
            throw Exception("MM6 indoor chest {} has invalid active grid", i);
        current.chests[i].inventoryMatrix = live.inventoryMatrix;
        current.chests[i].flags = live.flags;
        for (size_t j = 0; j < live.items.size(); ++j)
            std::memcpy(&current.chests[i].items[j], &live.items[j], 24);
    }
    for (size_t i = 0; i < current.doors.size(); ++i) {
        BLVDoor_MM7 live{};
        snapshot(location.doors[i], &live);
        if (live.numVertices != current.doors[i].numVertices ||
            live.numFaces != current.doors[i].numFaces ||
            live.numSectors != current.doors[i].numSectors ||
            live.numOffsets != current.doors[i].numOffsets)
            throw Exception("MM6 indoor door {} geometry counts changed", i);
        current.doors[i].attributes = live.attributes;
        current.doors[i].timeSinceTriggered = live.timeSinceTriggered;
        current.doors[i].state = live.state;
    }
    size_t offset = 0;
    for (const BLVDoor_MM7 &door : current.doors) {
        size_t identifiers = size_t(door.numVertices) + size_t(door.numFaces) + size_t(door.numSectors);
        size_t skippedDeltas = 2 * size_t(door.numFaces);
        size_t offsets = 3 * size_t(door.numOffsets);
        if (offset + identifiers + skippedDeltas + offsets > current.doorsData.size() ||
            !std::equal(current.doorsData.begin() + offset, current.doorsData.begin() + offset + identifiers,
                        location.doorsData.begin() + offset) ||
            !std::equal(current.doorsData.begin() + offset + identifiers + skippedDeltas,
                        current.doorsData.begin() + offset + identifiers + skippedDeltas + offsets,
                        location.doorsData.begin() + offset + identifiers + skippedDeltas))
            throw Exception("MM6 indoor door geometry arrays changed");
        offset += identifiers + skippedDeltas + offsets;
    }
    return encodeMM6IndoorRuntimeDelta(current, base);
}
