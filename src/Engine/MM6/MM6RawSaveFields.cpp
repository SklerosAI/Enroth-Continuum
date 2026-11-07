#include "MM6RawSaveFields.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include "MM6SaveCrossFields.h"

namespace mm6 {
namespace {

constexpr std::array<std::array<char, 4>, 19> REQUIRED_TAGS = {{
    {'P', 'A', 'R', 'T'}, {'C', 'H', 'A', 'R'}, {'Q', 'E', 'S', 'T'}, {'E', 'F', 'F', 'T'},
    {'I', 'N', 'V', 'R'}, {'H', 'E', 'A', 'D'}, {'T', 'I', 'M', 'E'}, {'U', 'I', 'S', 'T'},
    {'V', 'I', 'S', 'U'}, {'M', 'A', 'P', 'S'}, {'R', 'E', 'S', 'C'}, {'R', 'S', 'T', 'S'},
    {'H', 'E', 'L', 'D'}, {'B', 'U', 'F', 'F'}, {'H', 'I', 'R', 'E'}, {'R', 'C', 'V', 'R'},
    {'C', 'H', 'S', 'T'}, {'S', 'C', 'L', 'K'}, {'S', 'H', 'O', 'P'},
}};

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 raw save requires MM6 title");
}

void checkSectionSet(const SaveContainer &container) {
    if (container.version != 2 || container.sections.size() != REQUIRED_TAGS.size())
        throw std::runtime_error("MM6 raw save requires nineteen v2 sections");
    for (const SaveSection &section : container.sections) {
        if (!section.required ||
            std::find(REQUIRED_TAGS.begin(), REQUIRED_TAGS.end(), section.tag) == REQUIRED_TAGS.end())
            throw std::runtime_error("MM6 raw save has optional or unsupported section");
    }
    for (const auto &tag : REQUIRED_TAGS)
        if (std::none_of(container.sections.begin(), container.sections.end(),
                         [&tag](const SaveSection &section) { return section.tag == tag; }))
            throw std::runtime_error("MM6 raw save is missing a required section");
}

void checkCrossFields(const RawSaveFields &fields) {
    validateSharedSaveCrossFields(fields.core, fields.metadata, fields.maps,
                                  fields.recovery, fields.chest);
    if (!fields.inventory.ready || !fields.shops.ready())
        throw std::runtime_error("MM6 raw save requires ready inventory and shop owners");
}

} // namespace

std::vector<std::uint8_t> encodeRawSave(const RawSaveFields &fields, GameTitle title) {
    requireMM6(title);
    checkCrossFields(fields);
    auto core = encodeCoreSections(fields.core);
    auto progress = encodeDurableProgressSections(fields.bits, fields.effects);
    SaveSection inventory = encodeRawInventorySection(fields.inventory);
    auto metadata = encodeMetadataSections(fields.metadata);
    SaveSection maps = encodeMapSection(fields.maps);
    SaveSection npcs = encodeNpcSection(fields.npcs);
    SaveSection resistances = encodeResistanceSection(fields.resistances);
    SaveSection heldItem = encodeRawHeldItemSection(fields.heldItem, title);
    SaveSection buffs = encodeBuffSection(fields.buffs);
    SaveSection hirelings = encodeHirelingSection(fields.hirelings);
    SaveSection recovery = encodeRecoverySection(fields.recovery);
    SaveSection chest = encodeChestSection(fields.chest);
    SaveSection clocks = encodeShopGuildClockSection(fields.shops.clocks);
    SaveSection stock = encodeShopStockSection(fields.shops.stock);

    SaveContainer container;
    container.version = 2;
    container.sections.reserve(REQUIRED_TAGS.size());
    for (SaveSection &section : core)
        container.sections.push_back(std::move(section));
    for (SaveSection &section : progress)
        container.sections.push_back(std::move(section));
    container.sections.push_back(std::move(inventory));
    for (SaveSection &section : metadata)
        container.sections.push_back(std::move(section));
    container.sections.push_back(std::move(maps));
    container.sections.push_back(std::move(npcs));
    container.sections.push_back(std::move(resistances));
    container.sections.push_back(std::move(heldItem));
    container.sections.push_back(std::move(buffs));
    container.sections.push_back(std::move(hirelings));
    container.sections.push_back(std::move(recovery));
    container.sections.push_back(std::move(chest));
    container.sections.push_back(std::move(clocks));
    container.sections.push_back(std::move(stock));
    checkSectionSet(container);
    return encodeSaveContainer(container);
}

RawSaveFields decodeRawSave(std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(bytes);
    checkSectionSet(container); // Reject v1 or unexpected fields before decoding owners.
    RawSaveFields fields;
    fields.core = decodeCoreSections(container);
    auto [bits, effects] = decodeDurableProgressSections(container);
    fields.bits = std::move(bits);
    fields.effects = std::move(effects);
    fields.inventory = decodeRawInventorySection(container);
    fields.metadata = decodeMetadataSections(container);
    fields.maps = decodeMapSection(container);
    fields.npcs = decodeNpcSection(container);
    fields.resistances = decodeResistanceSection(container);
    fields.heldItem = decodeRawHeldItemSection(container, title);
    fields.buffs = decodeBuffSection(container);
    fields.hirelings = decodeHirelingSection(container);
    fields.recovery = decodeRecoverySection(container);
    fields.chest = decodeChestSection(container);
    fields.shops.clocks = decodeShopGuildClockSection(container);
    fields.shops.stock = decodeShopStockSection(container);
    checkCrossFields(fields);
    return fields;
}

void loadRawSaveBeforeMutation(std::span<const std::uint8_t> bytes,
                               RawSaveFields *destination, GameTitle title) {
    requireMM6(title);
    if (!destination)
        throw std::runtime_error("null MM6 raw save destination");
    RawSaveFields staged = decodeRawSave(bytes, title);
    *destination = std::move(staged);
}

} // namespace mm6
