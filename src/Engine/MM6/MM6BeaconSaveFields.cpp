#include "MM6BeaconSaveFields.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "Core/Time/Time.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'B', 'E', 'A', 'C'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t BEACON_RGBA_SIZE = BEACON_IMAGE_WIDTH * BEACON_IMAGE_HEIGHT * 4;
constexpr std::size_t THUMBNAIL_RGBA_SIZE = SAVE_THUMBNAIL_WIDTH * SAVE_THUMBNAIL_HEIGHT * 4;
constexpr std::size_t MAX_MAP_NAME = 64;
constexpr std::size_t MAX_PAYLOAD_SIZE = 600000;

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 beacon section");
        return _bytes[_offset++];
    }

    std::uint16_t u16() {
        std::uint16_t result = u8();
        result |= std::uint16_t(u8()) << 8;
        return result;
    }

    std::uint32_t u32() {
        std::uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= std::uint32_t(u8()) << shift;
        return result;
    }

    std::uint64_t u64() {
        std::uint64_t result = 0;
        for (int shift = 0; shift < 64; shift += 8)
            result |= std::uint64_t(u8()) << shift;
        return result;
    }

    std::span<const std::uint8_t> take(std::size_t length) {
        if (length > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 beacon image or map name");
        std::span<const std::uint8_t> result = _bytes.subspan(_offset, length);
        _offset += length;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 beacon section bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

void put16(std::vector<std::uint8_t> *bytes, std::uint16_t value) {
    bytes->push_back(value);
    bytes->push_back(value >> 8);
}

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(value >> shift);
}

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        bytes->push_back(value >> shift);
}

bool validMapFileName(std::string_view name) {
    if (name.size() < 5 || name.size() > MAX_MAP_NAME ||
        (!name.ends_with(".blv") && !name.ends_with(".odm")))
        return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    });
}

void validate(const BeaconSaveFields &fields) {
    if (!fields.thumbnailRgba.empty() && fields.thumbnailRgba.size() != THUMBNAIL_RGBA_SIZE)
        throw std::runtime_error("invalid MM6 save thumbnail dimensions");
    for (const auto &character : fields.beacons) {
        for (const auto &slot : character) {
            if (!slot)
                continue;
            if (slot->expiresAtTicks <= 0 || slot->expiresAtTicks > Time::fromYears(10000).ticks() ||
                !validMapFileName(slot->mapFileName) || slot->rgba.size() != BEACON_RGBA_SIZE ||
                std::any_of(slot->position.begin(), slot->position.end(), [](float value) {
                    return !std::isfinite(value) || std::abs(value) > 10000000;
                }))
                throw std::runtime_error("invalid MM6 beacon state or image");
        }
    }
}

} // namespace

SaveSection encodeBeaconSection(const BeaconSaveFields &fields) {
    validate(fields);
    SaveSection section{TAG, true, {VERSION, 4, 5, static_cast<std::uint8_t>(!fields.thumbnailRgba.empty())}};
    if (!fields.thumbnailRgba.empty())
        section.payload.insert(section.payload.end(), fields.thumbnailRgba.begin(), fields.thumbnailRgba.end());
    for (const auto &character : fields.beacons) {
        for (const auto &slot : character) {
            section.payload.push_back(slot ? 1 : 0);
            if (!slot)
                continue;
            put64(&section.payload, std::bit_cast<std::uint64_t>(slot->expiresAtTicks));
            section.payload.push_back(slot->mapFileName.size());
            section.payload.insert(section.payload.end(), slot->mapFileName.begin(), slot->mapFileName.end());
            for (float component : slot->position)
                put32(&section.payload, std::bit_cast<std::uint32_t>(component));
            put16(&section.payload, std::bit_cast<std::uint16_t>(slot->yaw));
            put16(&section.payload, std::bit_cast<std::uint16_t>(slot->pitch));
            put16(&section.payload, slot->unknown);
            section.payload.insert(section.payload.end(), slot->rgba.begin(), slot->rgba.end());
        }
    }
    if (section.payload.size() > MAX_PAYLOAD_SIZE)
        throw std::runtime_error("MM6 beacon section exceeds size limit");
    return section;
}

BeaconSaveFields decodeBeaconSection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag != TAG)
            continue;
        if (section)
            throw std::runtime_error("duplicate MM6 BEAC section");
        section = &candidate;
    }
    if (!section || !section->required || section->payload.size() > MAX_PAYLOAD_SIZE)
        throw std::runtime_error("missing or invalid MM6 BEAC section");
    Reader reader(section->payload);
    if (reader.u8() != VERSION || reader.u8() != 4 || reader.u8() != 5)
        throw std::runtime_error("unsupported MM6 BEAC version or shape");
    const std::uint8_t hasThumbnail = reader.u8();
    if (hasThumbnail > 1)
        throw std::runtime_error("invalid MM6 thumbnail flag");
    BeaconSaveFields fields;
    if (hasThumbnail) {
        std::span<const std::uint8_t> pixels = reader.take(THUMBNAIL_RGBA_SIZE);
        fields.thumbnailRgba.assign(pixels.begin(), pixels.end());
    }
    for (auto &character : fields.beacons) {
        for (auto &slot : character) {
            const std::uint8_t present = reader.u8();
            if (present > 1)
                throw std::runtime_error("invalid MM6 beacon presence flag");
            if (!present)
                continue;
            BeaconSaveRecord record;
            record.expiresAtTicks = std::bit_cast<std::int64_t>(reader.u64());
            const std::size_t nameLength = reader.u8();
            const std::span<const std::uint8_t> name = reader.take(nameLength);
            record.mapFileName.assign(name.begin(), name.end());
            for (float &component : record.position)
                component = std::bit_cast<float>(reader.u32());
            record.yaw = std::bit_cast<std::int16_t>(reader.u16());
            record.pitch = std::bit_cast<std::int16_t>(reader.u16());
            record.unknown = reader.u16();
            const std::span<const std::uint8_t> pixels = reader.take(BEACON_RGBA_SIZE);
            record.rgba.assign(pixels.begin(), pixels.end());
            slot = std::move(record);
        }
    }
    reader.finish();
    validate(fields);
    return fields;
}

void loadBeaconFieldsBeforeMutation(std::span<const std::uint8_t> bytes,
                                    BeaconSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 beacon destination");
    BeaconSaveFields staged = decodeBeaconSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
