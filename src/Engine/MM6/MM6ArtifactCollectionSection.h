#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

namespace mm6 {

/**
 * The All Artifacts mod's ledger of the random artifacts 400..429 the party collected.
 */
struct ArtifactCollection {
    bool initialized = false;
    std::array<std::uint8_t, 30> collected = {};

    friend bool operator==(const ArtifactCollection &, const ArtifactCollection &) = default;
};

/**
 * @param collection                    The party's artifact ledger.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Optional `ARTC` section, or `std::nullopt` for a ledger the mod never started,
 *                                      which a save leaves out. Throws for a flag other than 0 or 1.
 */
std::optional<SaveSection> encodeArtifactCollectionSection(const ArtifactCollection &collection, GameTitle title);

/**
 * @param container                     Container with at most one `ARTC` section.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Decoded ledger, not initialized for a save without the section. Throws on a
 *                                      malformed section.
 */
ArtifactCollection decodeArtifactCollectionSection(const SaveContainer &container, GameTitle title);

} // namespace mm6
