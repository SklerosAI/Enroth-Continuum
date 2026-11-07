#include "MM6V5DecorationRuntimeFields.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/DecorationTable.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'D', 'E', 'C', 'R'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t MAX_DECORATIONS = 16384;
constexpr std::size_t RECORD_BYTES = 30;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 DECR requires MM6 title");
}

void requireMapName(std::string_view name) {
    if (name.empty() || name.size() > 63 || !name.ends_with(".odm") ||
        !std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                   c == '_' || c == '.';
        }))
        throw std::runtime_error("MM6 DECR map name is not canonical outdoor identity");
}

void put16(std::vector<std::uint8_t> *out, std::uint16_t value) {
    out->push_back(value);
    out->push_back(value >> 8);
}

void put32(std::vector<std::uint8_t> *out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(value >> shift);
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}
    std::uint8_t u8() {
        if (_offset >= _bytes.size()) throw std::runtime_error("truncated MM6 DECR");
        return _bytes[_offset++];
    }
    std::uint16_t u16() { return std::uint16_t(u8()) | (std::uint16_t(u8()) << 8); }
    std::uint32_t u32() {
        std::uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= std::uint32_t(u8()) << shift;
        return result;
    }
    void finish() const {
        if (_offset != _bytes.size()) throw std::runtime_error("MM6 DECR trailing bytes");
    }
 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

void requireRuntimeFields(const LevelDecoration &decoration) {
    if (!std::isfinite(decoration.vPosition.x) ||
        !std::isfinite(decoration.vPosition.y) ||
        !std::isfinite(decoration.vPosition.z) ||
        decoration.eventVarId < 0 || decoration.eventVarId >= 124)
        throw std::runtime_error("MM6 DECR has unsafe runtime field");
}

bool sameFloat(float lhs, float rhs) {
    return std::bit_cast<std::uint32_t>(lhs) == std::bit_cast<std::uint32_t>(rhs);
}

bool samePosition(const Vec3f &lhs, const Vec3f &rhs) {
    return sameFloat(lhs.x, rhs.x) && sameFloat(lhs.y, rhs.y) &&
           sameFloat(lhs.z, rhs.z);
}

} // namespace

DecorationRuntimeSaveFields captureDecorationRuntimeFields(
    std::string_view mapName, const Blob &decodedBase,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title) {
    DecorationRuntimeSaveFields result;
    result.mapName = mapName;
    result.decorations.assign(live.begin(), live.end());
    requireDecorationRuntimeBaseAgreement(
        result, mapName, decodedBase, events, table, title);
    (void) encodeDecorationRuntimeSection(result, title);
    return result;
}

SaveSection encodeDecorationRuntimeSection(const DecorationRuntimeSaveFields &fields,
                                           GameTitle title) {
    requireMM6(title);
    requireMapName(fields.mapName);
    if (fields.decorations.size() > MAX_DECORATIONS)
        throw std::runtime_error("MM6 DECR count exceeds limit");
    std::vector<std::uint8_t> bytes;
    bytes.reserve(2 + fields.mapName.size() + 4 + fields.decorations.size() * RECORD_BYTES);
    bytes.push_back(VERSION);
    bytes.push_back(fields.mapName.size());
    bytes.insert(bytes.end(), fields.mapName.begin(), fields.mapName.end());
    put32(&bytes, fields.decorations.size());
    for (const LevelDecoration &decoration : fields.decorations) {
        requireRuntimeFields(decoration);
        put32(&bytes, std::bit_cast<std::uint32_t>(
            static_cast<std::int32_t>(std::to_underlying(decoration.uDecorationDescID))));
        put16(&bytes, std::to_underlying(decoration.uFlags));
        put32(&bytes, std::bit_cast<std::uint32_t>(decoration.vPosition.x));
        put32(&bytes, std::bit_cast<std::uint32_t>(decoration.vPosition.y));
        put32(&bytes, std::bit_cast<std::uint32_t>(decoration.vPosition.z));
        put32(&bytes, std::bit_cast<std::uint32_t>(decoration._yawAngle));
        put16(&bytes, decoration.uCog);
        put16(&bytes, decoration.uEventID);
        put16(&bytes, decoration.uTriggerRange);
        put16(&bytes, std::bit_cast<std::uint16_t>(decoration.eventVarId));
    }
    return {TAG, true, std::move(bytes)};
}

DecorationRuntimeSaveFields decodeDecorationRuntimeSection(
    const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections)
        if (candidate.tag == TAG) {
            if (section) throw std::runtime_error("duplicate MM6 DECR section");
            section = &candidate;
        }
    if (!section || !section->required || section->payload.size() < 6 ||
        section->payload.size() > 2 + 63 + 4 + MAX_DECORATIONS * RECORD_BYTES)
        throw std::runtime_error("missing or unbounded MM6 DECR section");
    Reader input(section->payload);
    if (input.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 DECR version");
    const auto nameSize = input.u8();
    DecorationRuntimeSaveFields result;
    for (std::size_t index = 0; index < nameSize; ++index)
        result.mapName.push_back(input.u8());
    requireMapName(result.mapName);
    const std::uint32_t count = input.u32();
    if (count > MAX_DECORATIONS ||
        section->payload.size() != 2 + nameSize + 4 + count * RECORD_BYTES)
        throw std::runtime_error("MM6 DECR count differs from payload");
    result.decorations.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        LevelDecoration decoration;
        decoration.uDecorationDescID = static_cast<DecorationId>(
            std::bit_cast<std::int32_t>(input.u32()));
        decoration.uFlags = LevelDecorationFlags(input.u16());
        const float x = std::bit_cast<float>(input.u32());
        const float y = std::bit_cast<float>(input.u32());
        const float z = std::bit_cast<float>(input.u32());
        decoration.vPosition = Vec3f(x, y, z);
        decoration._yawAngle = std::bit_cast<std::int32_t>(input.u32());
        decoration.uCog = input.u16();
        decoration.uEventID = input.u16();
        decoration.uTriggerRange = input.u16();
        decoration.eventVarId = std::bit_cast<std::int16_t>(input.u16());
        requireRuntimeFields(decoration);
        result.decorations.push_back(decoration);
    }
    input.finish();
    if (encodeDecorationRuntimeSection(result, title) != *section)
        throw std::runtime_error("MM6 DECR is not canonical");
    return result;
}

void requireDecorationRuntimeBaseAgreement(
    const DecorationRuntimeSaveFields &fields, std::string_view mapName,
    const Blob &decodedBase, const EventRuntimeMapEntry &events,
    const DecorationTable &table, GameTitle title) {
    requireMM6(title);
    requireMapName(mapName);
    if (mapName != "oute3.odm")
        throw std::runtime_error("MM6 DECR source replay requires first outdoor map");
    if (fields.mapName != mapName || events.mapName != mapName || decodedBase.empty())
        throw std::runtime_error("MM6 DECR lacks active source map join");
    OutdoorLocation_MM7 base;
    deserializeMM6Outdoor(decodedBase, &base);
    requireDecorationRuntimeBaseAgreement(fields, mapName, base, events, table, title);
}

void requireDecorationRuntimeBaseAgreement(
    const DecorationRuntimeSaveFields &fields, std::string_view mapName,
    const OutdoorLocation_MM7 &base, const EventRuntimeMapEntry &events,
    const DecorationTable &table, GameTitle title) {
    requireMM6(title);
    requireMapName(mapName);
    if (mapName != "oute3.odm" || fields.mapName != mapName ||
        events.mapName != mapName)
        throw std::runtime_error("MM6 DECR lacks active source map join");
    if (base.decorations.size() != fields.decorations.size() ||
        base.decorationNames.size() != fields.decorations.size())
        throw std::runtime_error("MM6 DECR count differs from source ODM");
    std::vector<LevelDecoration> source(fields.decorations.size());
    for (std::size_t index = 0; index < source.size(); ++index) {
        reconstruct(base.decorations[index], &source[index]);
        const auto &name = base.decorationNames[index];
        const auto end = std::find(name.begin(), name.end(), '\0');
        source[index].uDecorationDescID = table.decorationId(
            std::string(name.begin(), end));
        if (source[index].uDecorationDescID == DECORATION_NULL)
            throw std::runtime_error("MM6 DECR source descriptor lacks template");
        const LevelDecoration &runtime = fields.decorations[index];
        requireRuntimeFields(runtime);
        const auto descriptor = std::to_underlying(runtime.uDecorationDescID);
        if (descriptor <= 0 || static_cast<std::size_t>(descriptor) >= table.decorations.size())
            throw std::runtime_error("MM6 DECR runtime descriptor lacks template");
        if (!samePosition(source[index].vPosition, runtime.vPosition) ||
            source[index]._yawAngle != runtime._yawAngle ||
            source[index].uCog != runtime.uCog ||
            source[index].uEventID != runtime.uEventID ||
            source[index].uTriggerRange != runtime.uTriggerRange)
            throw std::runtime_error("MM6 DECR source slot anchor changed at slot " +
                                     std::to_string(index));
    }
    if (captureEventRuntimeDecorationBindings(source, title) != events.bindings)
        throw std::runtime_error("MM6 DECR EVRT bindings differ from source ODM");
}

} // namespace mm6
