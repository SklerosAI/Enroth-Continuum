#include "MM6IndoorSnapshot.h"

#include <algorithm>
#include <cstdint>
#include <utility>

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
