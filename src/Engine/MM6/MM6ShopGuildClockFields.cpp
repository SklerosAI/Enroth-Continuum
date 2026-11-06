#include "MM6ShopGuildClockFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> SCLK_TAG = {'S', 'C', 'L', 'K'};
constexpr std::size_t PAYLOAD_SIZE = 1 + 68 * 8;

void requireReadyMM6(const ShopGuildClockFields &owner, GameTitle title) {
    if (title != GameTitle::MM6 || !owner.ready)
        throw std::runtime_error("MM6 shop/guild clocks require a ready title-local owner");
}

} // namespace

std::size_t shopClockSlot(unsigned row) {
    if (row < 1 || row > 47)
        throw std::runtime_error("MM6 shop refresh row is outside 1..47");
    return row - 1;
}

std::size_t guildClockSlot(unsigned row) {
    if (row < 119 || row > 140)
        throw std::runtime_error("MM6 spell-guild refresh row is outside 119..140");
    return 46 + (row - 119);
}

ShopGuildClockFields captureShopGuildClocks(const ShopGuildClockFields &owner, GameTitle title) {
    requireReadyMM6(owner, title);
    return owner;
}

SaveSection encodeShopGuildClockSection(const ShopGuildClockFields &fields) {
    if (!fields.ready)
        throw std::runtime_error("MM6 SCLK requires a ready title-local owner");
    SaveSection section{SCLK_TAG, true, {}};
    section.payload.reserve(PAYLOAD_SIZE);
    section.payload.push_back(1);
    for (std::uint64_t word : fields.deadlineWords)
        for (int shift = 0; shift < 64; shift += 8)
            section.payload.push_back(static_cast<std::uint8_t>(word >> shift));
    return section;
}

ShopGuildClockFields decodeShopGuildClockSection(const SaveContainer &container) {
    auto it = std::find_if(container.sections.begin(), container.sections.end(),
                           [](const SaveSection &section) { return section.tag == SCLK_TAG; });
    if (it == container.sections.end() || !it->required || it->payload.size() != PAYLOAD_SIZE ||
        it->payload[0] != 1)
        throw std::runtime_error("MM6 save lacks a valid required SCLK section");
    ShopGuildClockFields fields;
    fields.ready = true;
    std::size_t offset = 1;
    for (std::uint64_t &word : fields.deadlineWords) {
        word = 0;
        for (int shift = 0; shift < 64; shift += 8)
            word |= std::uint64_t(it->payload[offset++]) << shift;
    }
    return fields;
}

void loadShopGuildClocksBeforeMutation(std::span<const std::uint8_t> bytes,
                                      ShopGuildClockFields *destination, GameTitle title) {
    if (title != GameTitle::MM6 || !destination)
        throw std::runtime_error("MM6 SCLK load requires a title-local destination");
    ShopGuildClockFields staged = decodeShopGuildClockSection(decodeSaveContainer(bytes));
    *destination = staged;
}

} // namespace mm6
