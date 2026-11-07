#include "MM6V3SaveFields.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include "MM6SpawnedActorSection.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> SPAW_TAG = {'S', 'P', 'A', 'W'};
constexpr std::array<char, 4> GNPC_TAG = {'G', 'N', 'P', 'C'};
constexpr std::array<char, 4> RNGS_TAG = {'R', 'N', 'G', 'S'};
constexpr std::array<std::array<char, 4>, 22> REQUIRED_TAGS = {{
    {'P', 'A', 'R', 'T'}, {'C', 'H', 'A', 'R'}, {'Q', 'E', 'S', 'T'}, {'E', 'F', 'F', 'T'},
    {'I', 'N', 'V', 'R'}, {'H', 'E', 'A', 'D'}, {'T', 'I', 'M', 'E'}, {'U', 'I', 'S', 'T'},
    {'V', 'I', 'S', 'U'}, {'M', 'A', 'P', 'S'}, {'R', 'E', 'S', 'C'}, {'R', 'S', 'T', 'S'},
    {'H', 'E', 'L', 'D'}, {'B', 'U', 'F', 'F'}, {'H', 'I', 'R', 'E'}, {'R', 'C', 'V', 'R'},
    {'C', 'H', 'S', 'T'}, {'S', 'C', 'L', 'K'}, {'S', 'H', 'O', 'P'}, SPAW_TAG,
    GNPC_TAG, RNGS_TAG,
}};

bool isNewTag(const std::array<char, 4> &tag) {
    return tag == SPAW_TAG || tag == GNPC_TAG || tag == RNGS_TAG;
}

void checkSectionSet(const SaveContainer &container) {
    if (container.version != 3 || container.sections.size() != REQUIRED_TAGS.size())
        throw std::runtime_error("MM6 v3 save requires twenty-two sections");
    for (const SaveSection &section : container.sections) {
        if (!section.required ||
            std::find(REQUIRED_TAGS.begin(), REQUIRED_TAGS.end(), section.tag) == REQUIRED_TAGS.end())
            throw std::runtime_error("MM6 v3 save has optional or unsupported section");
    }
    for (const auto &tag : REQUIRED_TAGS)
        if (std::none_of(container.sections.begin(), container.sections.end(),
                         [&tag](const SaveSection &section) { return section.tag == tag; }))
            throw std::runtime_error("MM6 v3 save is missing a required section");
}

void validateSpawnedOwner(std::span<const std::uint8_t> payload) {
    if (payload.size() < 16)
        throw std::runtime_error("MM6 v3 spawned actor section is truncated");
    const size_t firstSlot = std::uint32_t(payload[8]) |
        (std::uint32_t(payload[9]) << 8) | (std::uint32_t(payload[10]) << 16) |
        (std::uint32_t(payload[11]) << 24);
    const auto actors = decodeSpawnedActorSection(payload, firstSlot, GameTitle::MM6);
    if (encodeSpawnedActorSection(actors, firstSlot, GameTitle::MM6) !=
        std::vector<std::uint8_t>(payload.begin(), payload.end()))
        throw std::runtime_error("MM6 v3 spawned actor section is not canonical");
}

const SaveSection &spawnedOwner(const SaveContainer &container) {
    const auto it = std::find_if(container.sections.begin(), container.sections.end(),
                                 [](const SaveSection &section) { return section.tag == SPAW_TAG; });
    if (it == container.sections.end())
        throw std::runtime_error("MM6 v3 spawned actor section is missing");
    return *it;
}

} // namespace

std::vector<std::uint8_t> encodeV3Save(const V3SaveFields &fields, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v3 save requires MM6 title");
    if (fields.generatedNpcs.mapName != fields.raw.metadata.header.mapFileName)
        throw std::runtime_error("MM6 v3 generated NPC map differs from HEAD");
    validateSpawnedOwner(fields.spawnedActorPayload);
    SaveContainer container = decodeSaveContainer(encodeRawSave(fields.raw, title));
    container.version = 3;
    container.sections.push_back({SPAW_TAG, true, fields.spawnedActorPayload});
    container.sections.push_back(encodeGeneratedNpcSection(fields.generatedNpcs, title));
    container.sections.push_back(encodeGameplayRngSection(fields.gameplayRng, title));
    checkSectionSet(container);
    return encodeSaveContainer(container);
}

V3SaveFields decodeV3Save(std::span<const std::uint8_t> bytes, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v3 save requires MM6 title");
    SaveContainer container = decodeSaveContainer(bytes);
    checkSectionSet(container); // Reject other versions and shapes before owner decoding.
    const SaveSection &spawned = spawnedOwner(container);
    validateSpawnedOwner(spawned.payload);
    V3SaveFields result;
    result.spawnedActorPayload = spawned.payload;
    result.generatedNpcs = decodeGeneratedNpcSection(container, title);
    result.gameplayRng = decodeGameplayRngSection(container, title);

    SaveContainer raw;
    raw.version = 2;
    raw.sections.reserve(19);
    for (SaveSection &section : container.sections)
        if (!isNewTag(section.tag))
            raw.sections.push_back(std::move(section));
    result.raw = decodeRawSave(encodeSaveContainer(raw), title);
    if (result.generatedNpcs.mapName != result.raw.metadata.header.mapFileName)
        throw std::runtime_error("MM6 v3 generated NPC map differs from HEAD");
    return result;
}

} // namespace mm6
