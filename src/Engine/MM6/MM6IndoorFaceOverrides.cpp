#include "MM6IndoorFaceOverrides.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'I', 'F', 'O', 'V'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t MAX_MAPS = 52;
constexpr std::size_t MAX_FACES = 65535;
constexpr std::size_t MAX_NAME = 16;
constexpr std::size_t MAX_PAYLOAD = 2 * 1024 * 1024;
constexpr std::uint8_t ATTRIBUTES = 1;
constexpr std::uint8_t TEXTURE = 2;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("indoor face overrides require MM6 title");
}

bool validKey(std::string_view value, bool allowEmpty) {
    if (value.size() > MAX_NAME || (!allowEmpty && value.empty()))
        return false;
    return std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    });
}

bool validMapName(std::string_view value) {
    return value.size() >= 5 && value.size() <= MAX_NAME && value.ends_with(".blv") &&
           validKey(value.substr(0, value.size() - 4), false);
}

bool validTextureName(std::string_view value) {
    return value.size() <= MAX_NAME &&
           std::all_of(value.begin(), value.end(), [](char c) {
               return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                      c == '_' || c == '-' || c == '$';
           });
}

void validateFaces(std::span<const IndoorFaceState> faces) {
    if (faces.empty() || faces.size() > MAX_FACES)
        throw std::runtime_error("MM6 indoor face count is outside IFOV bounds");
    for (const IndoorFaceState &face : faces)
        if (!validTextureName(face.textureName))
            throw std::runtime_error("MM6 indoor face texture identity is not canonical");
}

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(static_cast<std::uint8_t>(value >> shift));
}

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        bytes->push_back(static_cast<std::uint8_t>(value >> shift));
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset >= _bytes.size())
            throw std::runtime_error("truncated MM6 indoor face override");
        return _bytes[_offset++];
    }

    std::uint32_t u32() {
        std::uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8)
            value |= std::uint32_t(u8()) << shift;
        return value;
    }

    std::uint64_t u64() {
        std::uint64_t value = 0;
        for (int shift = 0; shift < 64; shift += 8)
            value |= std::uint64_t(u8()) << shift;
        return value;
    }

    std::string name() {
        std::size_t length = u8();
        if (length > MAX_NAME || length > _bytes.size() - _offset)
            throw std::runtime_error("invalid MM6 indoor face override name length");
        std::string value(_bytes.begin() + _offset, _bytes.begin() + _offset + length);
        _offset += length;
        return value;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 indoor face override bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

std::uint64_t fingerprint(std::span<const IndoorFaceState> faces) {
    // Stable mismatch detector, not a cryptographic asset identity. The
    // separately validated MM6 map manifest remains the authoritative source.
    std::uint64_t hash = 14695981039346656037ull;
    const auto mix = [&](std::uint8_t value) {
        hash ^= value;
        hash *= 1099511628211ull;
    };
    for (const IndoorFaceState &face : faces) {
        for (int shift = 0; shift < 32; shift += 8)
            mix(static_cast<std::uint8_t>(face.attributes >> shift));
        mix(static_cast<std::uint8_t>(face.textureName.size()));
        for (char c : face.textureName)
            mix(static_cast<std::uint8_t>(c));
    }
    return hash;
}

void validate(const IndoorFaceOverrides &fields) {
    if (fields.maps.empty() || fields.maps.size() > MAX_MAPS)
        throw std::runtime_error("MM6 IFOV has invalid map count");
    std::string_view previousMap;
    std::size_t estimated = 2;
    for (const IndoorFaceMapOverrides &map : fields.maps) {
        if (!validMapName(map.mapName) || (!previousMap.empty() && map.mapName <= previousMap) ||
            map.baseFaceCount == 0 || map.baseFaceCount > MAX_FACES ||
            map.faces.empty() || map.faces.size() > map.baseFaceCount)
            throw std::runtime_error("MM6 IFOV has invalid map entry");
        previousMap = map.mapName;
        estimated += 1 + map.mapName.size() + 4 + 8 + 4;
        std::optional<std::uint32_t> previousIndex;
        for (const IndoorFaceOverride &face : map.faces) {
            if (face.faceIndex >= map.baseFaceCount ||
                (previousIndex && face.faceIndex <= *previousIndex) ||
                (!face.attributes && !face.textureName) ||
                (face.textureName && !validTextureName(*face.textureName)))
                throw std::runtime_error("MM6 IFOV has invalid or duplicate face entry");
            previousIndex = face.faceIndex;
            estimated += 4 + 1 + (face.attributes ? 4 : 0) +
                         (face.textureName ? 1 + face.textureName->size() : 0);
            if (estimated > MAX_PAYLOAD)
                throw std::runtime_error("MM6 IFOV exceeds section size limit");
        }
    }
}

} // namespace

std::optional<IndoorFaceMapOverrides> captureIndoorFaceOverrides(
    std::string_view mapName, std::span<const IndoorFaceState> base,
    std::span<const IndoorFaceState> current, GameTitle title) {
    requireMM6(title);
    if (!validMapName(mapName) || base.size() != current.size())
        throw std::runtime_error("MM6 indoor face capture has invalid map or face count");
    validateFaces(base);
    validateFaces(current);
    IndoorFaceMapOverrides result;
    result.mapName = mapName;
    result.baseFaceCount = base.size();
    result.baseFingerprint = fingerprint(base);
    for (std::size_t i = 0; i < base.size(); ++i) {
        IndoorFaceOverride changed;
        changed.faceIndex = i;
        if (base[i].attributes != current[i].attributes)
            changed.attributes = current[i].attributes;
        if (base[i].textureName != current[i].textureName)
            changed.textureName = current[i].textureName;
        if (changed.attributes || changed.textureName)
            result.faces.push_back(std::move(changed));
    }
    if (result.faces.empty())
        return std::nullopt;
    validate(IndoorFaceOverrides{{result}});
    return result;
}

SaveSection encodeIndoorFaceOverrideSection(const IndoorFaceOverrides &fields) {
    validate(fields);
    SaveSection section{TAG, false, {VERSION, static_cast<std::uint8_t>(fields.maps.size())}};
    for (const IndoorFaceMapOverrides &map : fields.maps) {
        section.payload.push_back(static_cast<std::uint8_t>(map.mapName.size()));
        section.payload.insert(section.payload.end(), map.mapName.begin(), map.mapName.end());
        put32(&section.payload, map.baseFaceCount);
        put64(&section.payload, map.baseFingerprint);
        put32(&section.payload, map.faces.size());
        for (const IndoorFaceOverride &face : map.faces) {
            put32(&section.payload, face.faceIndex);
            section.payload.push_back((face.attributes ? ATTRIBUTES : 0) |
                                      (face.textureName ? TEXTURE : 0));
            if (face.attributes)
                put32(&section.payload, *face.attributes);
            if (face.textureName) {
                section.payload.push_back(static_cast<std::uint8_t>(face.textureName->size()));
                section.payload.insert(section.payload.end(), face.textureName->begin(),
                                       face.textureName->end());
            }
        }
    }
    return section;
}

IndoorFaceOverrides decodeIndoorFaceOverrideSection(const SaveContainer &container) {
    if (std::count_if(container.sections.begin(), container.sections.end(),
                      [](const SaveSection &section) { return section.tag == TAG; }) > 1)
        throw std::runtime_error("duplicate MM6 IFOV section");
    auto it = std::find_if(container.sections.begin(), container.sections.end(),
                           [](const SaveSection &section) { return section.tag == TAG; });
    if (it == container.sections.end())
        return {};
    if (it->required || it->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("MM6 IFOV must be a bounded optional section");
    Reader reader(it->payload);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 indoor face override version");
    std::size_t mapCount = reader.u8();
    if (mapCount == 0 || mapCount > MAX_MAPS)
        throw std::runtime_error("MM6 IFOV has invalid map count");
    IndoorFaceOverrides fields;
    fields.maps.reserve(mapCount);
    for (std::size_t i = 0; i < mapCount; ++i) {
        IndoorFaceMapOverrides map;
        map.mapName = reader.name();
        map.baseFaceCount = reader.u32();
        map.baseFingerprint = reader.u64();
        std::size_t faceCount = reader.u32();
        if (map.baseFaceCount == 0 || map.baseFaceCount > MAX_FACES ||
            faceCount == 0 || faceCount > map.baseFaceCount)
            throw std::runtime_error("MM6 IFOV has invalid face entry count");
        map.faces.reserve(faceCount);
        for (std::size_t j = 0; j < faceCount; ++j) {
            IndoorFaceOverride face;
            face.faceIndex = reader.u32();
            std::uint8_t flags = reader.u8();
            if (flags == 0 || (flags & ~(ATTRIBUTES | TEXTURE)))
                throw std::runtime_error("MM6 IFOV has unsupported face entry flags");
            if (flags & ATTRIBUTES)
                face.attributes = reader.u32();
            if (flags & TEXTURE)
                face.textureName = reader.name();
            map.faces.push_back(std::move(face));
        }
        fields.maps.push_back(std::move(map));
    }
    reader.finish();
    validate(fields);
    return fields;
}

IndoorFaceOverrides stageIndoorFaceOverrides(const IndoorFaceOverrides &fields,
                                             const IndoorBaseFaceReader &readBase,
                                             GameTitle title) {
    requireMM6(title);
    if (fields.maps.empty())
        return {};
    validate(fields);
    if (!readBase)
        throw std::runtime_error("MM6 IFOV lacks a base-face reader");
    for (const IndoorFaceMapOverrides &map : fields.maps) {
        std::vector<IndoorFaceState> base = readBase(map.mapName);
        validateFaces(base);
        if (base.size() != map.baseFaceCount || fingerprint(base) != map.baseFingerprint)
            throw std::runtime_error("MM6 IFOV base faces differ from saved map identity");
        for (const IndoorFaceOverride &face : map.faces) {
            if ((face.attributes && *face.attributes == base[face.faceIndex].attributes) ||
                (face.textureName && *face.textureName == base[face.faceIndex].textureName))
                throw std::runtime_error("MM6 IFOV contains an unchanged face field");
        }
    }
    return fields;
}

void loadIndoorFaceOverridesBeforeMutation(std::span<const std::uint8_t> bytes,
                                           const IndoorBaseFaceReader &readBase,
                                           IndoorFaceOverrides *destination,
                                           GameTitle title) {
    requireMM6(title);
    if (!destination)
        throw std::runtime_error("null MM6 IFOV destination");
    SaveContainer container = decodeSaveContainer(bytes);
    IndoorFaceOverrides candidate = stageIndoorFaceOverrides(
        decodeIndoorFaceOverrideSection(container), readBase, title);
    *destination = std::move(candidate);
}

void applyIndoorFaceOverridesAfterMapReload(const IndoorFaceOverrides &staged,
                                            std::string_view mapName,
                                            std::vector<IndoorFaceState> *faces,
                                            GameTitle title) {
    requireMM6(title);
    if (!faces || !validMapName(mapName))
        throw std::runtime_error("MM6 IFOV has invalid apply destination");
    if (staged.maps.empty())
        return;
    validate(staged);
    auto it = std::find_if(staged.maps.begin(), staged.maps.end(),
                           [mapName](const IndoorFaceMapOverrides &map) { return map.mapName == mapName; });
    if (it == staged.maps.end())
        return;
    if (faces->size() != it->baseFaceCount)
        throw std::runtime_error("MM6 IFOV target face count changed after staging");
    std::vector<IndoorFaceState> candidate = *faces;
    validateFaces(candidate);
    for (const IndoorFaceOverride &face : it->faces) {
        if (face.attributes)
            candidate[face.faceIndex].attributes = *face.attributes;
        if (face.textureName)
            candidate[face.faceIndex].textureName = *face.textureName;
    }
    faces->swap(candidate);
}

} // namespace mm6
