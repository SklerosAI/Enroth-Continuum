#include "MM6BuffSaveFields.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Core/Time/Duration.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> BUFF_TAG = {'B', 'U', 'F', 'F'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t RECORD_SIZE = 15;
constexpr size_t PAYLOAD_SIZE = 4 + (20 + 4 * 24) * RECORD_SIZE;
constexpr std::int64_t GAME_RATE = Duration::GAME_SECONDS_IN_REALTIME_SECOND;
constexpr std::int64_t MAX_SAFE_TICKS = std::numeric_limits<std::int64_t>::max() / GAME_RATE;

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset >= _bytes.size())
            throw std::runtime_error("truncated MM6 BUFF field");
        return _bytes[_offset++];
    }

    std::uint16_t u16() {
        std::uint16_t result = u8();
        return result | (std::uint16_t(u8()) << 8);
    }

    std::uint64_t u64() {
        std::uint64_t result = 0;
        for (int shift = 0; shift < 64; shift += 8)
            result |= std::uint64_t(u8()) << shift;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 BUFF bytes");
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

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        put8(bytes, value >> shift);
}

void validate(const SavedSpellBuff &buff) {
    if (buff.expireTicks < -1 || buff.expireTicks > MAX_SAFE_TICKS || buff.mastery > 3 ||
        buff.overlayId > 50 || buff.caster < -1 || buff.caster > 3)
        throw std::runtime_error("invalid MM6 timed buff field");
}

void validate(const BuffSaveFields &fields) {
    for (const SavedSpellBuff &buff : fields.party)
        validate(buff);
    for (const auto &character : fields.characters)
        for (const SavedSpellBuff &buff : character)
            validate(buff);
}

void putBuff(std::vector<std::uint8_t> *bytes, const SavedSpellBuff &buff) {
    put64(bytes, std::bit_cast<std::uint64_t>(buff.expireTicks));
    put16(bytes, buff.power);
    put8(bytes, buff.mastery);
    put16(bytes, buff.overlayId);
    put8(bytes, std::bit_cast<std::uint8_t>(buff.caster));
    put8(bytes, buff.isGM ? 1 : 0);
}

SavedSpellBuff readBuff(Reader *reader) {
    SavedSpellBuff buff;
    buff.expireTicks = std::bit_cast<std::int64_t>(reader->u64());
    buff.power = reader->u16();
    buff.mastery = reader->u8();
    buff.overlayId = reader->u16();
    buff.caster = std::bit_cast<std::int8_t>(reader->u8());
    const std::uint8_t isGM = reader->u8();
    if (isGM > 1)
        throw std::runtime_error("invalid MM6 BUFF source flag");
    buff.isGM = isGM;
    validate(buff);
    return buff;
}

} // namespace

SaveSection encodeBuffSection(const BuffSaveFields &fields) {
    validate(fields);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(PAYLOAD_SIZE);
    put8(&bytes, VERSION);
    put8(&bytes, fields.party.size());
    put8(&bytes, fields.characters.size());
    put8(&bytes, fields.characters[0].size());
    for (const SavedSpellBuff &buff : fields.party)
        putBuff(&bytes, buff);
    for (const auto &character : fields.characters)
        for (const SavedSpellBuff &buff : character)
            putBuff(&bytes, buff);
    return {BUFF_TAG, true, std::move(bytes)};
}

BuffSaveFields decodeBuffSection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == BUFF_TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 BUFF section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() != PAYLOAD_SIZE)
        throw std::runtime_error("missing or malformed MM6 BUFF section");
    Reader reader(section->payload);
    if (reader.u8() != VERSION || reader.u8() != 20 || reader.u8() != 4 || reader.u8() != 24)
        throw std::runtime_error("unsupported MM6 BUFF version or count");
    BuffSaveFields fields;
    for (SavedSpellBuff &buff : fields.party)
        buff = readBuff(&reader);
    for (auto &character : fields.characters)
        for (SavedSpellBuff &buff : character)
            buff = readBuff(&reader);
    reader.finish();
    return fields;
}

void loadBuffFieldsBeforeMutation(std::span<const std::uint8_t> bytes, BuffSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 buff save destination");
    BuffSaveFields staged = decodeBuffSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
