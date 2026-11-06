#include "MM6MapSaveFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mm6 {
namespace {

constexpr std::array<char, 4> MAPS_TAG = {'M', 'A', 'P', 'S'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t MAX_MAPS = 67;
constexpr size_t MAX_DELTA_SIZE = 8 * 1024 * 1024;
constexpr size_t MAX_PAYLOAD_SIZE = 32 * 1024 * 1024;

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(value >> shift);
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 map save field");
        return _bytes[_offset++];
    }

    std::uint32_t u32() {
        std::uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= std::uint32_t(u8()) << shift;
        return result;
    }

    std::span<const std::uint8_t> take(size_t size) {
        if (size > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 map delta bytes");
        std::span<const std::uint8_t> result = _bytes.subspan(_offset, size);
        _offset += size;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 map save field bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    size_t _offset = 0;
};

bool validName(std::string_view name) {
    if (name.size() < 5 || name.size() > 16 ||
        (!name.ends_with(".ddm") && !name.ends_with(".dlv")))
        return false;
    return std::all_of(name.begin(), name.end() - 4, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    });
}

void validate(const MapSaveFields &fields) {
    if (fields.deltas.empty() || fields.deltas.size() > MAX_MAPS)
        throw std::runtime_error("MM6 map save has invalid delta count");
    std::unordered_set<std::string> names;
    size_t total = 2;
    for (const SavedMapDelta &delta : fields.deltas) {
        if (!validName(delta.name) || !names.insert(delta.name).second ||
            delta.bytes.empty() || delta.bytes.size() > MAX_DELTA_SIZE)
            throw std::runtime_error("MM6 map save has invalid delta name or size");
        total += 1 + delta.name.size() + 4 + delta.bytes.size();
        if (total > MAX_PAYLOAD_SIZE)
            throw std::runtime_error("MM6 map save section exceeds size limit");
    }
}

} // namespace

SaveSection encodeMapSection(const MapSaveFields &fields) {
    validate(fields);
    SaveSection section{MAPS_TAG, true, {}};
    section.payload.push_back(VERSION);
    section.payload.push_back(fields.deltas.size());
    for (const SavedMapDelta &delta : fields.deltas) {
        section.payload.push_back(delta.name.size());
        section.payload.insert(section.payload.end(), delta.name.begin(), delta.name.end());
        put32(&section.payload, delta.bytes.size());
        section.payload.insert(section.payload.end(), delta.bytes.begin(), delta.bytes.end());
    }
    return section;
}

MapSaveFields decodeMapSection(const SaveContainer &container) {
    auto it = std::find_if(container.sections.begin(), container.sections.end(),
                           [](const SaveSection &section) { return section.tag == MAPS_TAG; });
    if (it == container.sections.end() || !it->required || it->payload.size() > MAX_PAYLOAD_SIZE)
        throw std::runtime_error("MM6 save lacks a bounded required MAPS section");

    Reader reader(it->payload);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 map save field version");
    size_t count = reader.u8();
    if (count == 0 || count > MAX_MAPS)
        throw std::runtime_error("MM6 map save has invalid delta count");
    MapSaveFields fields;
    fields.deltas.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        size_t nameSize = reader.u8();
        std::span<const std::uint8_t> nameBytes = reader.take(nameSize);
        std::string name(nameBytes.begin(), nameBytes.end());
        size_t length = reader.u32();
        if (length == 0 || length > MAX_DELTA_SIZE)
            throw std::runtime_error("MM6 map delta exceeds size limit");
        std::span<const std::uint8_t> data = reader.take(length);
        fields.deltas.push_back({std::move(name), {data.begin(), data.end()}});
    }
    reader.finish();
    validate(fields);
    return fields;
}

void loadMapFieldsBeforeMutation(std::span<const std::uint8_t> bytes, MapSaveFields *destination) {
    SaveContainer container = decodeSaveContainer(bytes);
    MapSaveFields candidate = decodeMapSection(container);
    *destination = std::move(candidate);
}

} // namespace mm6
