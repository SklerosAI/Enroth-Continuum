#include "MM6ArtifactCollectionSection.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace mm6 {

static constexpr std::array<char, 4> ARTC = {'A', 'R', 'T', 'C'};
static constexpr std::uint8_t VERSION = 1;

static void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 ARTC requires the MM6 title");
}

static void requireFlags(const std::array<std::uint8_t, 30> &flags) {
    for (std::uint8_t flag : flags)
        if (flag > 1)
            throw std::runtime_error("MM6 ARTC flag is not 0 or 1");
}

std::optional<SaveSection> encodeArtifactCollectionSection(const ArtifactCollection &collection, GameTitle title) {
    requireMM6(title);
    if (!collection.initialized)
        return std::nullopt;
    requireFlags(collection.collected);
    SaveSection section{ARTC, false, {VERSION}};
    section.payload.insert(section.payload.end(), collection.collected.begin(), collection.collected.end());
    return section;
}

ArtifactCollection decodeArtifactCollectionSection(const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag != ARTC)
            continue;
        if (section)
            throw std::runtime_error("duplicate MM6 ARTC section");
        section = &candidate;
    }
    ArtifactCollection result;
    if (!section)
        return result;

    const std::vector<std::uint8_t> &bytes = section->payload;
    if (section->required || bytes.size() != 1 + result.collected.size() || bytes[0] != VERSION)
        throw std::runtime_error("malformed MM6 ARTC section");
    std::copy(bytes.begin() + 1, bytes.end(), result.collected.begin());
    requireFlags(result.collected);
    result.initialized = true;
    return result;
}

} // namespace mm6
