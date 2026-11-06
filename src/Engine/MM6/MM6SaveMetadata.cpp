#include "MM6SaveMetadata.h"

#include "MM6CoreSaveFields.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> HEAD_TAG = {'H', 'E', 'A', 'D'};
constexpr std::array<char, 4> TIME_TAG = {'T', 'I', 'M', 'E'};
constexpr std::array<char, 4> UIST_TAG = {'U', 'I', 'S', 'T'};
constexpr std::array<char, 4> VISU_TAG = {'V', 'I', 'S', 'U'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t MAX_DISPLAY_NAME = 63;
constexpr size_t MAX_MAP_NAME = 31;
constexpr size_t VISU_SIZE = 2 + 50 * 18;

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset >= _bytes.size())
            throw std::runtime_error("truncated MM6 save metadata");
        return _bytes[_offset++];
    }

    std::uint16_t u16() {
        std::uint16_t result = u8();
        return result | (std::uint16_t(u8()) << 8);
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

    std::string string(size_t count) {
        if (count > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 save metadata string");
        std::string result(reinterpret_cast<const char *>(_bytes.data() + _offset), count);
        _offset += count;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 save metadata bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    size_t _offset = 0;
};

void put8(std::vector<std::uint8_t> *bytes, std::uint8_t value) { bytes->push_back(value); }

void put16(std::vector<std::uint8_t> *bytes, std::uint16_t value) {
    put8(bytes, value);
    put8(bytes, value >> 8);
}

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        put8(bytes, value >> shift);
}

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        put8(bytes, value >> shift);
}

bool validDisplayName(const std::string &name) {
    return !name.empty() && name.size() <= MAX_DISPLAY_NAME &&
           std::none_of(name.begin(), name.end(), [](unsigned char c) { return c < 0x20 || c == 0x7f; });
}

bool validMapName(const std::string &name) {
    if (name.empty() || name.size() > MAX_MAP_NAME ||
        !(name.ends_with(".blv") || name.ends_with(".odm")))
        return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    });
}

void validate(const SaveMetadataFields &fields) {
    if (!validDisplayName(fields.header.displayName) || !validMapName(fields.header.mapFileName) ||
        fields.header.playingTicks < 0 || fields.timer.elapsedTicks < 0 ||
        fields.ui.activeCharacter < -1 || fields.ui.activeCharacter > 3 || fields.ui.resetTransientUi != 1)
        throw std::runtime_error("invalid MM6 save metadata field");
    for (const SaveOverlayField &overlay : fields.overlays)
        if (overlay.animationLength > 0 && overlay.tableIndex < 0)
            throw std::runtime_error("active MM6 overlay has negative table index");
}

std::vector<std::uint8_t> encodeHeader(const SaveHeaderFields &header) {
    std::vector<std::uint8_t> bytes;
    put8(&bytes, VERSION);
    put8(&bytes, header.displayName.size());
    bytes.insert(bytes.end(), header.displayName.begin(), header.displayName.end());
    put8(&bytes, header.mapFileName.size());
    bytes.insert(bytes.end(), header.mapFileName.begin(), header.mapFileName.end());
    put64(&bytes, std::bit_cast<std::uint64_t>(header.playingTicks));
    return bytes;
}

SaveHeaderFields decodeHeader(std::span<const std::uint8_t> bytes) {
    if (bytes.size() > 1 + 1 + MAX_DISPLAY_NAME + 1 + MAX_MAP_NAME + 8)
        throw std::runtime_error("MM6 HEAD field exceeds size limit");
    Reader reader(bytes);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 HEAD field version");
    SaveHeaderFields header;
    header.displayName = reader.string(reader.u8());
    header.mapFileName = reader.string(reader.u8());
    header.playingTicks = std::bit_cast<std::int64_t>(reader.u64());
    reader.finish();
    return header;
}

std::vector<std::uint8_t> encodeTimer(const SaveTimerFields &timer) {
    std::vector<std::uint8_t> bytes;
    put8(&bytes, VERSION);
    put64(&bytes, std::bit_cast<std::uint64_t>(timer.elapsedTicks));
    return bytes;
}

SaveTimerFields decodeTimer(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 9)
        throw std::runtime_error("invalid MM6 TIME field size");
    Reader reader(bytes);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 TIME field version");
    SaveTimerFields result{std::bit_cast<std::int64_t>(reader.u64())};
    reader.finish();
    return result;
}

std::vector<std::uint8_t> encodeUi(const SaveUiFields &ui) {
    return {VERSION, std::bit_cast<std::uint8_t>(ui.activeCharacter), ui.resetTransientUi};
}

SaveUiFields decodeUi(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 3 || bytes[0] != VERSION)
        throw std::runtime_error("invalid MM6 UIST field size or version");
    return {std::bit_cast<std::int8_t>(bytes[1]), bytes[2]};
}

std::vector<std::uint8_t> encodeOverlays(const std::array<SaveOverlayField, 50> &overlays) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(VISU_SIZE);
    put8(&bytes, VERSION);
    put8(&bytes, overlays.size());
    for (const SaveOverlayField &overlay : overlays) {
        put16(&bytes, std::bit_cast<std::uint16_t>(overlay.tableIndex));
        put16(&bytes, std::bit_cast<std::uint16_t>(overlay.frameTime));
        put16(&bytes, std::bit_cast<std::uint16_t>(overlay.animationLength));
        put16(&bytes, std::bit_cast<std::uint16_t>(overlay.x));
        put16(&bytes, std::bit_cast<std::uint16_t>(overlay.y));
        put16(&bytes, overlay.packedPid);
        put16(&bytes, std::bit_cast<std::uint16_t>(overlay.projectedSize));
        put32(&bytes, std::bit_cast<std::uint32_t>(overlay.damageModifier));
    }
    return bytes;
}

std::array<SaveOverlayField, 50> decodeOverlays(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != VISU_SIZE)
        throw std::runtime_error("invalid MM6 VISU field size");
    Reader reader(bytes);
    if (reader.u8() != VERSION || reader.u8() != 50)
        throw std::runtime_error("unsupported MM6 VISU field version or count");
    std::array<SaveOverlayField, 50> result;
    for (SaveOverlayField &overlay : result) {
        overlay.tableIndex = std::bit_cast<std::int16_t>(reader.u16());
        overlay.frameTime = std::bit_cast<std::int16_t>(reader.u16());
        overlay.animationLength = std::bit_cast<std::int16_t>(reader.u16());
        overlay.x = std::bit_cast<std::int16_t>(reader.u16());
        overlay.y = std::bit_cast<std::int16_t>(reader.u16());
        overlay.packedPid = reader.u16();
        overlay.projectedSize = std::bit_cast<std::int16_t>(reader.u16());
        overlay.damageModifier = std::bit_cast<std::int32_t>(reader.u32());
    }
    reader.finish();
    return result;
}

} // namespace

std::array<SaveSection, 4> encodeMetadataSections(const SaveMetadataFields &fields) {
    validate(fields);
    return {SaveSection{HEAD_TAG, true, encodeHeader(fields.header)},
            SaveSection{TIME_TAG, true, encodeTimer(fields.timer)},
            SaveSection{UIST_TAG, true, encodeUi(fields.ui)},
            SaveSection{VISU_TAG, true, encodeOverlays(fields.overlays)}};
}

SaveMetadataFields decodeMetadataSections(const SaveContainer &container) {
    const SaveSection *head = nullptr;
    const SaveSection *time = nullptr;
    const SaveSection *uist = nullptr;
    const SaveSection *visu = nullptr;
    for (const SaveSection &section : container.sections) {
        if (section.tag == HEAD_TAG) head = &section;
        if (section.tag == TIME_TAG) time = &section;
        if (section.tag == UIST_TAG) uist = &section;
        if (section.tag == VISU_TAG) visu = &section;
    }
    if (!head || !time || !uist || !visu || !head->required || !time->required ||
        !uist->required || !visu->required)
        throw std::runtime_error("MM6 save metadata fields are missing or optional");

    SaveMetadataFields result;
    result.header = decodeHeader(head->payload);
    result.timer = decodeTimer(time->payload);
    result.ui = decodeUi(uist->payload);
    result.overlays = decodeOverlays(visu->payload);
    validate(result);
    return result;
}

void validateMetadataAgainstCore(const SaveMetadataFields &metadata, const CoreSaveFields &core) {
    if (metadata.header.playingTicks != core.party.playingTicks ||
        metadata.ui.activeCharacter != core.party.activeCharacter)
        throw std::runtime_error("MM6 metadata disagrees with core party fields");
}

void loadMetadataBeforeMutation(std::span<const std::uint8_t> bytes, SaveMetadataFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 save metadata destination");
    SaveMetadataFields staged = decodeMetadataSections(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
