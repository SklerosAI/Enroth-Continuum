#include "MM6OutdoorWire.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mm6 {
namespace {

constexpr size_t MAX_DECODED_SIZE = 64 * 1024 * 1024;
constexpr size_t ODM_HEADER_SIZE = 160 + 16 + 3 * 128 * 128;
constexpr size_t MODEL_HEADER_SIZE = 188;
constexpr size_t DECORATION_MAP_SIZE = 128 * 128 * 4;
constexpr size_t OUTDOOR_VISIBILITY_SIZE = 2 * 88 * 11;

class Cursor {
 public:
    explicit Cursor(std::span<const std::uint8_t> bytes) : _bytes(bytes) {
        if (bytes.size() > MAX_DECODED_SIZE)
            throw std::runtime_error("MM6 outdoor payload exceeds size limit");
    }

    size_t position() const { return _offset; }
    size_t remaining() const { return _bytes.size() - _offset; }

    void take(size_t count) {
        if (count > remaining())
            throw std::runtime_error("truncated MM6 outdoor payload");
        _offset += count;
    }

    std::uint32_t read32() {
        if (remaining() < 4)
            throw std::runtime_error("truncated MM6 outdoor count");
        const std::uint32_t result = std::uint32_t(_bytes[_offset]) |
                                     (std::uint32_t(_bytes[_offset + 1]) << 8) |
                                     (std::uint32_t(_bytes[_offset + 2]) << 16) |
                                     (std::uint32_t(_bytes[_offset + 3]) << 24);
        _offset += 4;
        return result;
    }

    std::uint32_t countAndTake(size_t width, std::uint32_t limit) {
        const std::uint32_t count = read32();
        if (count > limit || count > remaining() / width)
            throw std::runtime_error("MM6 outdoor record count exceeds bounds");
        take(size_t(count) * width);
        return count;
    }

    void finish() const {
        if (remaining() != 0)
            throw std::runtime_error("MM6 outdoor payload has trailing bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    size_t _offset = 0;
};

std::uint32_t read32(std::span<const std::uint8_t> bytes, size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4)
        throw std::runtime_error("truncated MM6 outdoor model header");
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}

WireSpan capture(Cursor *cursor, size_t count) {
    const size_t start = cursor->position();
    cursor->take(count);
    return {start, count};
}

WireSpan captureCounted(Cursor *cursor, size_t width, std::uint32_t limit, std::uint32_t *count) {
    const size_t start = cursor->position();
    *count = cursor->countAndTake(width, limit);
    return {start, cursor->position() - start};
}

WireSpan modelPart(const OutdoorBaseWire &wire, size_t modelIndex, size_t facetIndex, bool texture) {
    if (modelIndex >= wire.modelFaces.size() || facetIndex >= wire.modelFaces[modelIndex].count)
        throw std::runtime_error("MM6 outdoor model or facet index exceeds bounds");
    const WireSpan &range = texture ? wire.modelFaces[modelIndex].textureNames : wire.modelFaces[modelIndex].faceRecords;
    const size_t width = texture ? 10 : 308;
    if (range.offset > wire.bytes.size() || range.size > wire.bytes.size() - range.offset ||
        range.size != size_t(wire.modelFaces[modelIndex].count) * width)
        throw std::runtime_error("MM6 outdoor model facet span is inconsistent");
    return {range.offset + facetIndex * width, width};
}

} // namespace

OutdoorBaseWire decodeOutdoorBase(std::span<const std::uint8_t> bytes) {
    Cursor cursor(bytes);
    cursor.take(ODM_HEADER_SIZE);
    const std::uint32_t modelCount = cursor.read32();
    if (modelCount > 2000 || modelCount > cursor.remaining() / MODEL_HEADER_SIZE)
        throw std::runtime_error("MM6 outdoor model count exceeds bounds");

    const size_t modelsOffset = cursor.position();
    cursor.take(size_t(modelCount) * MODEL_HEADER_SIZE);
    OutdoorBaseWire result;
    result.modelFaces.reserve(modelCount);
    for (size_t i = 0; i < modelCount; ++i) {
        const size_t offset = modelsOffset + i * MODEL_HEADER_SIZE;
        const std::uint32_t vertices = read32(bytes, offset + 68);
        const std::uint32_t faces = read32(bytes, offset + 76);
        const std::uint32_t bspNodes = read32(bytes, offset + 92);
        if (vertices > 100000 || faces > 100000 || bspNodes > 100000)
            throw std::runtime_error("MM6 outdoor model dimension exceeds limit");
        cursor.take(size_t(vertices) * 12);
        const WireSpan faceRecords = capture(&cursor, size_t(faces) * 308);
        cursor.take(size_t(faces) * 2);
        const WireSpan textureNames = capture(&cursor, size_t(faces) * 10);
        result.modelFaces.push_back({faceRecords, textureNames, faces});
        cursor.take(size_t(bspNodes) * 2 * 12); // Both legacy node lists stay in the source bytes.
    }

    const std::uint32_t decorations = cursor.countAndTake(28, 100000);
    cursor.take(size_t(decorations) * 32);
    const std::uint32_t pids = cursor.countAndTake(2, 100000);
    cursor.take(DECORATION_MAP_SIZE);
    const std::uint32_t spawns = cursor.countAndTake(20, 100000);
    cursor.finish();

    result.bytes.assign(bytes.begin(), bytes.end());
    result.modelCount = modelCount;
    result.decorationCount = decorations;
    result.decorationPidCount = pids;
    result.spawnCount = spawns;
    return result;
}

std::span<const std::uint8_t> OutdoorBaseWire::faceBytes(size_t modelIndex, size_t facetIndex) const {
    const WireSpan range = modelPart(*this, modelIndex, facetIndex, false);
    return std::span(bytes).subspan(range.offset, range.size);
}

std::span<std::uint8_t> OutdoorBaseWire::faceBytes(size_t modelIndex, size_t facetIndex) {
    const WireSpan range = modelPart(*this, modelIndex, facetIndex, false);
    return std::span(bytes).subspan(range.offset, range.size);
}

std::span<const std::uint8_t> OutdoorBaseWire::textureNameBytes(size_t modelIndex, size_t facetIndex) const {
    const WireSpan range = modelPart(*this, modelIndex, facetIndex, true);
    return std::span(bytes).subspan(range.offset, range.size);
}

std::span<std::uint8_t> OutdoorBaseWire::textureNameBytes(size_t modelIndex, size_t facetIndex) {
    const WireSpan range = modelPart(*this, modelIndex, facetIndex, true);
    return std::span(bytes).subspan(range.offset, range.size);
}

OutdoorDeltaWire decodeOutdoorDelta(std::span<const std::uint8_t> bytes) {
    Cursor cursor(bytes);
    OutdoorDeltaWire result;
    result.parts[static_cast<size_t>(OutdoorDeltaPart::ResetHeader)] = capture(&cursor, 8);
    result.parts[static_cast<size_t>(OutdoorDeltaPart::Visibility)] = capture(&cursor, OUTDOOR_VISIBILITY_SIZE);
    result.parts[static_cast<size_t>(OutdoorDeltaPart::Actors)] = captureCounted(&cursor, 548, 100000, &result.actorCount);
    result.parts[static_cast<size_t>(OutdoorDeltaPart::GroundItems)] = captureCounted(&cursor, 100, 100000, &result.groundItemCount);
    result.parts[static_cast<size_t>(OutdoorDeltaPart::Chests)] = captureCounted(&cursor, 4204, 2000, &result.chestCount);
    result.parts[static_cast<size_t>(OutdoorDeltaPart::PersistentBytes)] = capture(&cursor, 200);
    result.parts[static_cast<size_t>(OutdoorDeltaPart::Tail)] = capture(&cursor, 56);
    cursor.finish();
    result.bytes.assign(bytes.begin(), bytes.end());
    return result;
}

std::span<const std::uint8_t> OutdoorDeltaWire::part(OutdoorDeltaPart part) const {
    const size_t index = static_cast<size_t>(part);
    if (index >= parts.size())
        throw std::runtime_error("invalid MM6 outdoor delta part");
    const WireSpan &range = parts[index];
    if (range.offset > bytes.size() || range.size > bytes.size() - range.offset)
        throw std::runtime_error("MM6 outdoor delta part exceeds payload");
    return std::span(bytes).subspan(range.offset, range.size);
}

std::span<std::uint8_t> OutdoorDeltaWire::part(OutdoorDeltaPart part) {
    const size_t index = static_cast<size_t>(part);
    if (index >= parts.size())
        throw std::runtime_error("invalid MM6 outdoor delta part");
    const WireSpan &range = parts[index];
    if (range.offset > bytes.size() || range.size > bytes.size() - range.offset)
        throw std::runtime_error("MM6 outdoor delta part exceeds payload");
    return std::span(bytes).subspan(range.offset, range.size);
}

std::vector<std::uint8_t> encodeOutdoorBase(const OutdoorBaseWire &wire) {
    (void) decodeOutdoorBase(wire.bytes);
    return wire.bytes;
}

std::vector<std::uint8_t> encodeOutdoorDelta(const OutdoorDeltaWire &wire) {
    (void) decodeOutdoorDelta(wire.bytes);
    return wire.bytes;
}

} // namespace mm6
