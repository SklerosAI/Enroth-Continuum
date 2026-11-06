#include "MM6HirelingSaveFields.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Core/Time/Duration.h"

#include "MM6NpcSaveFields.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> HIRE_TAG = {'H', 'I', 'R', 'E'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t MAX_NAME_BYTES = 1024;
constexpr size_t MAX_PAYLOAD = 2 + 2 * (2 + MAX_NPC_RECORD_BYTES + 2 + MAX_NAME_BYTES + 17);
constexpr std::int64_t GAME_RATE = Duration::GAME_SECONDS_IN_REALTIME_SECOND;
constexpr std::int64_t MAX_SAFE_TICKS = std::numeric_limits<std::int64_t>::max() / GAME_RATE;

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset >= _bytes.size())
            throw std::runtime_error("truncated MM6 HIRE field");
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

    std::span<const std::uint8_t> take(size_t length) {
        if (length > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 HIRE record");
        std::span<const std::uint8_t> result = _bytes.subspan(_offset, length);
        _offset += length;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 HIRE bytes");
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

void validateName(const std::string &name) {
    if (name.size() > MAX_NAME_BYTES || name.find('\0') != std::string::npos)
        throw std::runtime_error("invalid MM6 hireling display name");
}

void validateSacrifice(const SavedSacrificeStatus &status) {
    if (status.elapsedTicks < 0 || status.elapsedTicks > MAX_SAFE_TICKS ||
        status.endTicks < 0 || status.endTicks > MAX_SAFE_TICKS ||
        (status.inProgress && status.endTicks == 0))
        throw std::runtime_error("invalid MM6 hireling sacrifice state");
}

void validate(const HirelingSaveFields &fields) {
    for (size_t index = 0; index < fields.hirelings.size(); ++index) {
        (void) encodeNpcRecord(fields.hirelings[index]);
        validateName(fields.displayNames[index]);
        validateSacrifice(fields.sacrifice[index]);
    }
}

} // namespace

SaveSection encodeHirelingSection(const HirelingSaveFields &fields) {
    validate(fields);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(MAX_PAYLOAD);
    put8(&bytes, VERSION);
    put8(&bytes, fields.hirelings.size());
    for (size_t index = 0; index < fields.hirelings.size(); ++index) {
        std::vector<std::uint8_t> npc = encodeNpcRecord(fields.hirelings[index]);
        put16(&bytes, npc.size());
        bytes.insert(bytes.end(), npc.begin(), npc.end());
        const std::string &name = fields.displayNames[index];
        put16(&bytes, name.size());
        bytes.insert(bytes.end(), name.begin(), name.end());
        const SavedSacrificeStatus &status = fields.sacrifice[index];
        put8(&bytes, status.inProgress ? 1 : 0);
        put64(&bytes, std::bit_cast<std::uint64_t>(status.elapsedTicks));
        put64(&bytes, std::bit_cast<std::uint64_t>(status.endTicks));
    }
    return {HIRE_TAG, true, std::move(bytes)};
}

HirelingSaveFields decodeHirelingSection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == HIRE_TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 HIRE section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("missing or oversized MM6 HIRE section");
    Reader reader(section->payload);
    if (reader.u8() != VERSION || reader.u8() != 2)
        throw std::runtime_error("unsupported MM6 HIRE version or count");
    HirelingSaveFields fields;
    for (size_t index = 0; index < fields.hirelings.size(); ++index) {
        const size_t npcLength = reader.u16();
        if (npcLength > MAX_NPC_RECORD_BYTES)
            throw std::runtime_error("MM6 hireling record exceeds size limit");
        fields.hirelings[index] = decodeNpcRecord(reader.take(npcLength));
        const size_t nameLength = reader.u16();
        if (nameLength > MAX_NAME_BYTES)
            throw std::runtime_error("MM6 hireling display name exceeds size limit");
        std::span<const std::uint8_t> name = reader.take(nameLength);
        fields.displayNames[index] = std::string(name.begin(), name.end());
        const std::uint8_t inProgress = reader.u8();
        if (inProgress > 1)
            throw std::runtime_error("invalid MM6 HIRE sacrifice flag");
        fields.sacrifice[index] = {bool(inProgress), std::bit_cast<std::int64_t>(reader.u64()),
                                  std::bit_cast<std::int64_t>(reader.u64())};
    }
    reader.finish();
    validate(fields);
    return fields;
}

void loadHirelingFieldsBeforeMutation(std::span<const std::uint8_t> bytes, HirelingSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 HIRE destination");
    HirelingSaveFields staged = decodeHirelingSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
