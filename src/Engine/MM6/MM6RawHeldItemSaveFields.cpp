#include "MM6RawHeldItemSaveFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'H', 'E', 'L', 'D'};
constexpr std::uint8_t VERSION = 2;
constexpr std::size_t HEADER_BYTES = 4;
constexpr std::size_t SECTION_BYTES = HEADER_BYTES + 28;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 raw held item requires MM6 title");
}

void validate(const RawHeldItemSaveFields &fields) {
    if (fields.present) {
        if (fields.record.itemNumberRaw() < 1 || fields.record.itemNumberRaw() > 580)
            throw std::runtime_error("MM6 raw held item has invalid source item number");
    } else {
        // Original cursor clear zeros bytes 0..24 and leaves bytes 25..27 intact.
        if (std::any_of(fields.record.begin(), fields.record.begin() + 25,
                        [](std::uint8_t byte) { return byte != 0; }))
            throw std::runtime_error("MM6 empty raw held item has nonzero cleared prefix");
    }
}

} // namespace

SaveSection encodeRawHeldItemSection(const RawHeldItemSaveFields &fields, GameTitle title) {
    requireMM6(title);
    validate(fields);
    SaveSection section{TAG, true, {VERSION, std::uint8_t(fields.present), 0, 0}};
    section.payload.insert(section.payload.end(), fields.record.begin(), fields.record.end());
    return section;
}

RawHeldItemSaveFields decodeRawHeldItemSection(const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 raw HELD section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() != SECTION_BYTES)
        throw std::runtime_error("missing or invalid MM6 raw HELD section");

    const auto &bytes = section->payload;
    if (bytes[0] != VERSION || bytes[1] > 1 || bytes[2] || bytes[3])
        throw std::runtime_error("unsupported MM6 raw HELD version or shape");

    RawHeldItemSaveFields result;
    result.present = bytes[1] != 0;
    std::copy(bytes.begin() + HEADER_BYTES, bytes.end(), result.record.begin());
    validate(result);
    return result;
}

void loadRawHeldItemBeforeMutation(std::span<const std::uint8_t> bytes,
                                   RawHeldItemSaveFields *destination, GameTitle title) {
    requireMM6(title);
    if (!destination)
        throw std::runtime_error("null MM6 raw held-item destination");
    RawHeldItemSaveFields staged = decodeRawHeldItemSection(decodeSaveContainer(bytes), title);
    *destination = std::move(staged);
}

} // namespace mm6
