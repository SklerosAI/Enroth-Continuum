#include "MM6EventRuntimeSaveFields.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6Decorations.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'E', 'V', 'R', 'T'};
constexpr std::uint8_t VERSION = 2;
constexpr std::size_t MAX_MAPS = 67;
constexpr std::size_t MAX_MAP_NAME = 63;
constexpr std::size_t MAX_BINDINGS = 124;
constexpr std::size_t BINDING_PAYLOAD = 27;
constexpr std::size_t MAX_ENTRY_PAYLOAD = 1 + MAX_MAP_NAME + 8 + 75 + 125 + 1 + MAX_BINDINGS * BINDING_PAYLOAD;
constexpr std::size_t MAX_PAYLOAD = 2 + MAX_MAPS * MAX_ENTRY_PAYLOAD;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 EVRT requires MM6 title");
}

bool validMapName(std::string_view name) {
    if (name.empty() || name.size() > MAX_MAP_NAME ||
        !(name.ends_with(".odm") || name.ends_with(".blv")))
        return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '.';
    });
}

void validate(const EventRuntimeSaveFields &fields) {
    if (fields.maps.empty() || fields.maps.size() > MAX_MAPS)
        throw std::runtime_error("MM6 EVRT has invalid map count");
    std::string_view previousName;
    for (const EventRuntimeMapEntry &entry : fields.maps) {
        if (!validMapName(entry.mapName) ||
            (!previousName.empty() && entry.mapName <= previousName) ||
            entry.respawnCount > std::numeric_limits<int>::max() ||
            entry.lastRespawnDay == 0 ||
            entry.lastRespawnDay > std::numeric_limits<int>::max() ||
            entry.bindings.size() > MAX_BINDINGS)
            throw std::runtime_error("MM6 EVRT map name, anchor or binding count is invalid");
        previousName = entry.mapName;
        std::uint32_t previousIndex = 0;
        for (std::size_t slot = 0; slot < entry.bindings.size(); ++slot) {
            const EventRuntimeDecorationBinding &binding = entry.bindings[slot];
            if ((slot && binding.decorationIndex <= previousIndex) ||
                binding.descriptorId == 0 || binding.eventVarId != slot)
                throw std::runtime_error("MM6 EVRT decoration bindings are not canonical");
            previousIndex = binding.decorationIndex;
        }
    }
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
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 EVRT section");
        return _bytes[_offset++];
    }

    std::uint16_t u16() {
        const std::uint16_t lo = u8();
        return lo | (std::uint16_t(u8()) << 8);
    }

    std::uint32_t u32() {
        const std::uint32_t lo = u16();
        return lo | (std::uint32_t(u16()) << 16);
    }

    std::span<const std::uint8_t> take(std::size_t length) {
        if (length > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 EVRT record");
        const auto result = _bytes.subspan(_offset, length);
        _offset += length;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 EVRT bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

} // namespace

std::vector<EventRuntimeDecorationBinding> captureEventRuntimeDecorationBindings(
    std::span<LevelDecoration> decorations, GameTitle title) {
    requireMM6(title);
    std::vector<EventRuntimeDecorationBinding> result;
    for (std::size_t index = 0; index < decorations.size(); ++index) {
        LevelDecoration &decoration = decorations[index];
        if (decoration.uEventID || !isInteractiveDecoration(decoration.uDecorationDescID))
            continue;
        if (result.size() == MAX_BINDINGS)
            break; // The loader never assigns a variable to later decorations.
        const auto descriptor = std::to_underlying(decoration.uDecorationDescID);
        if (index > std::numeric_limits<std::uint32_t>::max() ||
            descriptor <= 0 || descriptor > std::numeric_limits<std::uint16_t>::max())
            throw std::runtime_error("MM6 EVRT decoration identity is out of range");
        result.push_back({static_cast<std::uint32_t>(index),
                          static_cast<std::uint16_t>(descriptor),
                          decoration.uCog,
                          static_cast<std::uint8_t>(result.size()),
                          {std::bit_cast<std::uint32_t>(decoration.vPosition.x),
                           std::bit_cast<std::uint32_t>(decoration.vPosition.y),
                           std::bit_cast<std::uint32_t>(decoration.vPosition.z)},
                          decoration._yawAngle,
                          decoration.uTriggerRange});
    }
    return result;
}

SaveSection encodeEventRuntimeSection(const EventRuntimeSaveFields &fields,
                                      GameTitle title) {
    requireMM6(title);
    validate(fields);
    std::vector<std::uint8_t> payload;
    payload.reserve(2 + fields.maps.size() * 256);
    payload.push_back(VERSION);
    payload.push_back(static_cast<std::uint8_t>(fields.maps.size()));
    for (const EventRuntimeMapEntry &entry : fields.maps) {
        payload.push_back(static_cast<std::uint8_t>(entry.mapName.size()));
        payload.insert(payload.end(), entry.mapName.begin(), entry.mapName.end());
        put32(&payload, entry.respawnCount);
        put32(&payload, entry.lastRespawnDay);
        payload.insert(payload.end(), entry.mapVars.begin(), entry.mapVars.end());
        payload.insert(payload.end(), entry.decorVars.begin(), entry.decorVars.end());
        payload.push_back(static_cast<std::uint8_t>(entry.bindings.size()));
        for (const EventRuntimeDecorationBinding &binding : entry.bindings) {
            put32(&payload, binding.decorationIndex);
            put16(&payload, binding.descriptorId);
            put16(&payload, binding.cog);
            payload.push_back(binding.eventVarId);
            for (std::uint32_t bits : binding.positionBits)
                put32(&payload, bits);
            put32(&payload, std::bit_cast<std::uint32_t>(binding.yawAngle));
            put16(&payload, binding.triggerRange);
        }
    }
    return {TAG, true, std::move(payload)};
}

EventRuntimeSaveFields decodeEventRuntimeSection(const SaveContainer &container,
                                                 GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections)
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 EVRT section");
            section = &candidate;
        }
    if (!section || !section->required || section->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("missing or oversized MM6 EVRT section");
    Reader reader(section->payload);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 EVRT version");
    const std::size_t mapCount = reader.u8();
    if (mapCount == 0 || mapCount > MAX_MAPS)
        throw std::runtime_error("MM6 EVRT map count is invalid");
    EventRuntimeSaveFields result;
    result.maps.reserve(mapCount);
    for (std::size_t map = 0; map < mapCount; ++map) {
        EventRuntimeMapEntry entry;
        const std::size_t nameLength = reader.u8();
        const auto name = reader.take(nameLength);
        entry.mapName.assign(name.begin(), name.end());
        entry.respawnCount = reader.u32();
        entry.lastRespawnDay = reader.u32();
        const auto mapVars = reader.take(entry.mapVars.size());
        std::copy(mapVars.begin(), mapVars.end(), entry.mapVars.begin());
        const auto decorVars = reader.take(entry.decorVars.size());
        std::copy(decorVars.begin(), decorVars.end(), entry.decorVars.begin());
        const std::size_t bindingCount = reader.u8();
        if (bindingCount > MAX_BINDINGS)
            throw std::runtime_error("MM6 EVRT binding count is invalid");
        entry.bindings.reserve(bindingCount);
        for (std::size_t binding = 0; binding < bindingCount; ++binding) {
            EventRuntimeDecorationBinding decoded;
            decoded.decorationIndex = reader.u32();
            decoded.descriptorId = reader.u16();
            decoded.cog = reader.u16();
            decoded.eventVarId = reader.u8();
            for (std::uint32_t &bits : decoded.positionBits)
                bits = reader.u32();
            decoded.yawAngle = std::bit_cast<std::int32_t>(reader.u32());
            decoded.triggerRange = reader.u16();
            entry.bindings.push_back(decoded);
        }
        result.maps.push_back(std::move(entry));
    }
    reader.finish();
    validate(result);
    return result;
}

const EventRuntimeMapEntry &requireEventRuntimeMapEntry(
    const EventRuntimeSaveFields &fields, std::string_view mapName,
    std::uint32_t nativeRespawnCount, std::uint32_t nativeLastRespawnDay,
    std::span<LevelDecoration> decorations, GameTitle title) {
    requireMM6(title);
    validate(fields);
    const auto it = std::lower_bound(fields.maps.begin(), fields.maps.end(), mapName,
                                     [](const EventRuntimeMapEntry &entry, std::string_view name) {
                                         return entry.mapName < name;
                                     });
    if (it == fields.maps.end() || it->mapName != mapName ||
        it->respawnCount != nativeRespawnCount ||
        it->lastRespawnDay != nativeLastRespawnDay ||
        it->bindings != captureEventRuntimeDecorationBindings(decorations, title))
        throw std::runtime_error("MM6 EVRT map, respawn or decoration owner differs");
    return *it;
}

} // namespace mm6
