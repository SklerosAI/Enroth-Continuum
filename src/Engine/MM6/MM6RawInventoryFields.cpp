#include "MM6RawInventoryFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'I', 'N', 'V', 'R'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t CHARACTER_BYTES = RAW_INVENTORY_ITEM_SLOTS * 28 + (RAW_INVENTORY_GRID_CELLS + RAW_INVENTORY_EQUIPMENT_SLOTS) * 4;
constexpr std::size_t PAYLOAD_BYTES = 4 + 4 * CHARACTER_BYTES;
static_assert(PAYLOAD_BYTES == 17732);

void append32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(static_cast<std::uint8_t>(value >> shift));
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t *offset) {
    std::uint32_t result = 0;
    for (int shift = 0; shift < 32; shift += 8)
        result |= std::uint32_t(bytes[(*offset)++]) << shift;
    return result;
}

} // namespace

RawInventoryFields captureRawInventory(const RawInventoryFields &owner, GameTitle title) {
    if (title != GameTitle::MM6 || !owner.ready)
        throw std::runtime_error("MM6 raw inventory requires a ready title-local owner");
    return owner;
}

SaveSection encodeRawInventorySection(const RawInventoryFields &fields) {
    if (!fields.ready)
        throw std::runtime_error("MM6 INVR requires a ready title-local owner");
    SaveSection section{TAG, true, {}};
    section.payload.reserve(PAYLOAD_BYTES);
    section.payload.insert(section.payload.end(), {VERSION, 4, 0, 0});
    for (const RawCharacterInventory &character : fields.characters) {
        for (const ItemRecord28 &item : character.items)
            section.payload.insert(section.payload.end(), item.begin(), item.end());
        for (std::uint32_t word : character.gridWords)
            append32(&section.payload, word);
        for (std::uint32_t word : character.equipmentWords)
            append32(&section.payload, word);
    }
    return section;
}

RawInventoryFields decodeRawInventorySection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 INVR section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() != PAYLOAD_BYTES ||
        section->payload[0] != VERSION || section->payload[1] != 4 ||
        section->payload[2] || section->payload[3])
        throw std::runtime_error("MM6 save lacks a valid required INVR section");

    RawInventoryFields result;
    result.ready = true;
    const std::span<const std::uint8_t> bytes(section->payload);
    std::size_t offset = 4;
    for (RawCharacterInventory &character : result.characters) {
        for (ItemRecord28 &item : character.items) {
            std::copy_n(bytes.begin() + offset, item.size(), item.begin());
            offset += item.size();
        }
        for (std::uint32_t &word : character.gridWords)
            word = read32(bytes, &offset);
        for (std::uint32_t &word : character.equipmentWords)
            word = read32(bytes, &offset);
    }
    if (offset != bytes.size())
        throw std::runtime_error("MM6 INVR has trailing data");
    return result;
}

void loadRawInventoryBeforeMutation(std::span<const std::uint8_t> bytes,
                                    RawInventoryFields *destination, GameTitle title) {
    if (title != GameTitle::MM6 || !destination)
        throw std::runtime_error("MM6 INVR load requires a title-local destination");
    RawInventoryFields staged = decodeRawInventorySection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
