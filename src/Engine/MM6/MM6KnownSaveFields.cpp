#include "MM6KnownSaveFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<std::array<char, 4>, 11> REQUIRED_TAGS = {{
    {'P', 'A', 'R', 'T'}, {'C', 'H', 'A', 'R'}, {'Q', 'E', 'S', 'T'}, {'E', 'F', 'F', 'T'},
    {'I', 'T', 'E', 'M'}, {'H', 'E', 'A', 'D'}, {'T', 'I', 'M', 'E'}, {'U', 'I', 'S', 'T'},
    {'V', 'I', 'S', 'U'}, {'M', 'A', 'P', 'S'}, {'R', 'E', 'S', 'C'},
}};

void checkSectionSet(const SaveContainer &container) {
    if (container.sections.size() != REQUIRED_TAGS.size())
        throw std::runtime_error("MM6 known save requires exactly eleven sections");
    for (const SaveSection &section : container.sections) {
        if (!section.required ||
            std::find(REQUIRED_TAGS.begin(), REQUIRED_TAGS.end(), section.tag) == REQUIRED_TAGS.end())
            throw std::runtime_error("MM6 known save has optional or unsupported section");
    }
    for (const auto &tag : REQUIRED_TAGS)
        if (std::none_of(container.sections.begin(), container.sections.end(),
                         [&tag](const SaveSection &section) { return section.tag == tag; }))
            throw std::runtime_error("MM6 known save is missing a required section");
}

void checkCrossFields(const KnownSaveFields &fields) {
    validateMetadataAgainstCore(fields.metadata, fields.core);
    if (fields.maps.deltas.size() != 67)
        throw std::runtime_error("MM6 known save requires 67 map deltas");

    const std::string &mapName = fields.metadata.header.mapFileName;
    if (mapName.size() < 4)
        throw std::runtime_error("invalid MM6 active map name");
    const std::string deltaName = mapName.substr(0, mapName.size() - 4) +
        (mapName.ends_with(".blv") ? ".dlv" : ".ddm");
    if (std::none_of(fields.maps.deltas.begin(), fields.maps.deltas.end(),
                     [&deltaName](const SavedMapDelta &delta) { return delta.name == deltaName; }))
        throw std::runtime_error("MM6 active map has no saved delta companion");
}

} // namespace

std::vector<std::uint8_t> encodeKnownSave(const KnownSaveFields &fields) {
    auto core = encodeCoreSections(fields.core);
    auto durable = encodeDurableSections(fields.durable);
    auto metadata = encodeMetadataSections(fields.metadata);
    SaveSection maps = encodeMapSection(fields.maps);
    SaveSection npcs = encodeNpcSection(fields.npcs);
    checkCrossFields(fields);

    SaveContainer container;
    container.sections.reserve(REQUIRED_TAGS.size());
    for (SaveSection &section : core)
        container.sections.push_back(std::move(section));
    for (SaveSection &section : durable)
        container.sections.push_back(std::move(section));
    for (SaveSection &section : metadata)
        container.sections.push_back(std::move(section));
    container.sections.push_back(std::move(maps));
    container.sections.push_back(std::move(npcs));
    checkSectionSet(container);
    return encodeSaveContainer(container);
}

KnownSaveFields decodeKnownSave(std::span<const std::uint8_t> bytes) {
    SaveContainer container = decodeSaveContainer(bytes);
    checkSectionSet(container); // Reject optional extensions before consuming state.
    KnownSaveFields fields;
    fields.core = decodeCoreSections(container);
    fields.durable = decodeDurableSections(container);
    fields.metadata = decodeMetadataSections(container);
    fields.maps = decodeMapSection(container);
    fields.npcs = decodeNpcSection(container);
    checkCrossFields(fields);
    return fields;
}

void loadKnownSaveBeforeMutation(std::span<const std::uint8_t> bytes, KnownSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 known save destination");
    KnownSaveFields staged = decodeKnownSave(bytes);
    *destination = std::move(staged);
}

} // namespace mm6
