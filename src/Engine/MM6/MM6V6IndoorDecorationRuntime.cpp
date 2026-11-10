#include "MM6V6IndoorDecorationRuntime.h"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Tables/DecorationTable.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'I', 'D', 'R', 'T'};
constexpr std::size_t MAX_MAP_NAME = 63;
constexpr std::size_t MAX_DECORATIONS = 16384;
constexpr std::size_t RECORD_BYTES = 6;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 IDRT requires MM6 title");
}

void requireMapName(std::string_view name) {
    if (name.size() < 5 || name.size() > MAX_MAP_NAME || !name.ends_with(".blv") ||
        !std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                   c == '_' || c == '.';
        }))
        throw std::runtime_error("MM6 IDRT lacks canonical BLV identity");
}

void put16(std::vector<std::uint8_t> *bytes, std::uint16_t value) {
    bytes->push_back(value);
    bytes->push_back(value >> 8);
}

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(value >> shift);
}

std::uint16_t get16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || 2 > bytes.size() - offset)
        throw std::runtime_error("truncated MM6 IDRT section");
    return std::uint16_t(bytes[offset]) | std::uint16_t(bytes[offset + 1]) << 8;
}

std::uint32_t get32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return std::uint32_t(get16(bytes, offset)) |
           std::uint32_t(get16(bytes, offset + 2)) << 16;
}

std::vector<LevelDecoration> sourceDecorations(
    const IndoorLocation_MM7 &base, const DecorationTable &table) {
    if (base.decorations.size() > MAX_DECORATIONS ||
        base.decorations.size() != base.decorationNames.size())
        throw std::runtime_error("MM6 IDRT source decoration lists differ");
    std::vector<LevelDecoration> result(base.decorations.size());
    for (std::size_t index = 0; index < result.size(); ++index) {
        const auto &native = base.decorations[index];
        reconstruct(native, &result[index]);
        result[index].eventVarId = native.eventVarId; // MM6 has no MM7 75-offset.
        const auto &name = base.decorationNames[index];
        const auto end = std::find(name.begin(), name.end(), '\0');
        result[index].uDecorationDescID = table.decorationId(
            std::string(name.begin(), end));
        // The engine's BLV loader keeps DECORATION_NULL for empty or
        // unresolved source names; table slot zero is a real, indexed
        // sentinel used by indoor preparation. Preserve its slot identity.
    }
    return result;
}

bool sameImmutable(const LevelDecoration &lhs, const LevelDecoration &rhs) {
    return lhs.vPosition == rhs.vPosition && lhs._yawAngle == rhs._yawAngle &&
           lhs.uCog == rhs.uCog && lhs.uEventID == rhs.uEventID &&
           lhs.uTriggerRange == rhs.uTriggerRange;
}

void requireDescriptor(std::uint16_t id, const DecorationTable &table) {
    if (id >= table.decorations.size())
        throw std::runtime_error("MM6 IDRT runtime descriptor lacks template");
}

std::int16_t expectedOrdinal(std::size_t index,
                             const LevelDecoration &source,
                             const EventRuntimeMapEntry &events) {
    const auto it = std::find_if(events.bindings.begin(), events.bindings.end(),
        [index](const EventRuntimeDecorationBinding &entry) {
            return entry.decorationIndex == index;
        });
    return it == events.bindings.end() ? source.eventVarId : it->eventVarId;
}

} // namespace

IndoorDecorationRuntimeFields captureIndoorDecorationRuntime(
    std::string_view mapName, const IndoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title) {
    requireMM6(title);
    IndoorDecorationRuntimeFields result;
    result.mapName = mapName;
    result.records.reserve(live.size());
    for (const LevelDecoration &decoration : live) {
        const auto descriptor = std::to_underlying(decoration.uDecorationDescID);
        if (descriptor < 0 ||
            descriptor > std::numeric_limits<std::uint16_t>::max())
            throw std::runtime_error("MM6 IDRT runtime descriptor is out of range");
        result.records.push_back({static_cast<std::uint16_t>(descriptor),
                                  static_cast<std::uint16_t>(std::to_underlying(decoration.uFlags)),
                                  decoration.eventVarId});
    }
    requireIndoorDecorationRuntimeSourceAgreement(
        result, mapName, base, events, table, title);
    const auto source = sourceDecorations(base, table);
    for (std::size_t index = 0; index < live.size(); ++index)
        if (!sameImmutable(live[index], source[index]))
            throw std::runtime_error("MM6 IDRT live immutable slot differs from BLV");
    return result;
}

SaveSection encodeIndoorDecorationRuntimeSection(
    const IndoorDecorationRuntimeFields &fields, GameTitle title) {
    requireMM6(title);
    requireMapName(fields.mapName);
    if (fields.records.size() > MAX_DECORATIONS)
        throw std::runtime_error("MM6 IDRT exceeds bounded decoration count");
    SaveSection section{TAG, true, {}};
    section.payload.reserve(2 + fields.mapName.size() + 4 +
                            fields.records.size() * RECORD_BYTES);
    section.payload.push_back(1);
    section.payload.push_back(fields.mapName.size());
    section.payload.insert(section.payload.end(), fields.mapName.begin(), fields.mapName.end());
    put32(&section.payload, fields.records.size());
    for (const auto &record : fields.records) {
        put16(&section.payload, record.descriptorId);
        put16(&section.payload, record.flags);
        put16(&section.payload, std::bit_cast<std::uint16_t>(record.eventVarId));
    }
    return section;
}

IndoorDecorationRuntimeFields decodeIndoorDecorationRuntimeSection(
    const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const auto it = std::find_if(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == TAG; });
    if (it == container.sections.end() || !it->required || it->payload.size() < 6 ||
        it->payload.size() > 2 + MAX_MAP_NAME + 4 + MAX_DECORATIONS * RECORD_BYTES)
        throw std::runtime_error("missing or unbounded MM6 IDRT section");
    const auto &bytes = it->payload;
    const std::size_t nameLength = bytes[1];
    if (bytes[0] != 1 || nameLength > MAX_MAP_NAME ||
        2 + nameLength + 4 > bytes.size())
        throw std::runtime_error("MM6 IDRT has invalid section header");
    IndoorDecorationRuntimeFields result;
    result.mapName.assign(bytes.begin() + 2, bytes.begin() + 2 + nameLength);
    requireMapName(result.mapName);
    const std::uint32_t count = get32(bytes, 2 + nameLength);
    if (count > MAX_DECORATIONS ||
        bytes.size() != 2 + nameLength + 4 + count * RECORD_BYTES)
        throw std::runtime_error("MM6 IDRT count differs from payload");
    const std::size_t first = 2 + nameLength + 4;
    result.records.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const std::size_t offset = first + index * RECORD_BYTES;
        result.records.push_back({get16(bytes, offset), get16(bytes, offset + 2),
            std::bit_cast<std::int16_t>(get16(bytes, offset + 4))});
    }
    if (encodeIndoorDecorationRuntimeSection(result, title) != *it)
        throw std::runtime_error("MM6 IDRT is not canonical");
    return result;
}

void requireIndoorDecorationRuntimeSourceAgreement(
    const IndoorDecorationRuntimeFields &fields, std::string_view mapName,
    const IndoorLocation_MM7 &base, const EventRuntimeMapEntry &events,
    const DecorationTable &table, GameTitle title) {
    requireMM6(title);
    requireMapName(mapName);
    if (fields.mapName != mapName || events.mapName != mapName)
        throw std::runtime_error("MM6 IDRT lacks active BLV/EVRT identity");
    auto source = sourceDecorations(base, table);
    if (fields.records.size() != source.size() ||
        captureEventRuntimeDecorationBindings(source, title) !=
            events.bindings)
        throw std::runtime_error("MM6 IDRT source slots or EVRT bindings differ");
    for (std::size_t index = 0; index < source.size(); ++index) {
        const auto &record = fields.records[index];
        requireDescriptor(record.descriptorId, table);
        if (record.eventVarId != expectedOrdinal(index, source[index], events))
            throw std::runtime_error("MM6 IDRT event ordinal differs at slot " +
                                     std::to_string(index));
    }
}

std::vector<LevelDecoration> stageIndoorDecorationRuntime(
    const IndoorDecorationRuntimeFields &fields, const IndoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    GameTitle title) {
    requireIndoorDecorationRuntimeSourceAgreement(
        fields, events.mapName, base, events, table, title);
    auto result = sourceDecorations(base, table);
    for (std::size_t index = 0; index < result.size(); ++index) {
        const auto &record = fields.records[index];
        result[index].uDecorationDescID = static_cast<DecorationId>(record.descriptorId);
        result[index].uFlags = LevelDecorationFlags(record.flags);
        result[index].eventVarId = record.eventVarId;
    }
    return result;
}

void applyIndoorDecorationRuntimeAfterMapReload(
    const IndoorDecorationRuntimeFields &fields, const IndoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::vector<LevelDecoration> *afterEvent, GameTitle title) {
    if (!afterEvent)
        throw std::runtime_error("MM6 IDRT has null replay destination");
    const auto desired = stageIndoorDecorationRuntime(fields, base, events, table, title);
    if (afterEvent->size() != desired.size())
        throw std::runtime_error("MM6 IDRT post-event decoration count differs");
    auto result = *afterEvent;
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (!sameImmutable(result[index], desired[index]))
            throw std::runtime_error("MM6 IDRT post-event immutable slot differs");
        result[index].uDecorationDescID = desired[index].uDecorationDescID;
        result[index].uFlags = desired[index].uFlags;
        result[index].eventVarId = desired[index].eventVarId;
    }
    *afterEvent = std::move(result);
}

} // namespace mm6
