#include "MM6ResistanceSaveFields.h"

#include <bit>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'R', 'S', 'T', 'S'};
constexpr size_t HEADER_SIZE = 4;
constexpr size_t PAYLOAD_SIZE = HEADER_SIZE + 4 * 11 * 2 * 2;

void put16(std::vector<std::uint8_t> *bytes, std::int16_t value) {
    std::uint16_t raw = std::bit_cast<std::uint16_t>(value);
    bytes->push_back(raw & 0xffu);
    bytes->push_back(raw >> 8);
}

std::int16_t get16(std::span<const std::uint8_t> bytes, size_t *offset) {
    std::uint16_t raw = std::uint16_t(bytes[*offset]) |
                        (std::uint16_t(bytes[*offset + 1]) << 8);
    *offset += 2;
    return std::bit_cast<std::int16_t>(raw);
}

} // namespace

SaveSection encodeResistanceSection(const ResistanceSaveFields &fields) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(PAYLOAD_SIZE);
    bytes.insert(bytes.end(), {1, 4, 11, 0}); // Version, characters, channels, reserved.
    for (const ResistanceCharacterFields &character : fields.characters) {
        for (std::int16_t value : character.base)
            put16(&bytes, value);
        for (std::int16_t value : character.bonus)
            put16(&bytes, value);
    }
    return {TAG, true, std::move(bytes)};
}

ResistanceSaveFields decodeResistanceSection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 RSTS section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() != PAYLOAD_SIZE)
        throw std::runtime_error("missing or invalid MM6 RSTS section");
    std::span<const std::uint8_t> bytes = section->payload;
    if (bytes[0] != 1 || bytes[1] != 4 || bytes[2] != 11 || bytes[3] != 0)
        throw std::runtime_error("unsupported MM6 RSTS field version or shape");
    ResistanceSaveFields result;
    size_t offset = HEADER_SIZE;
    for (ResistanceCharacterFields &character : result.characters) {
        for (std::int16_t &value : character.base)
            value = get16(bytes, &offset);
        for (std::int16_t &value : character.bonus)
            value = get16(bytes, &offset);
    }
    return result;
}

void loadResistanceFieldsBeforeMutation(std::span<const std::uint8_t> bytes,
                                        ResistanceSaveFields *destination) {
    ResistanceSaveFields staged = decodeResistanceSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
