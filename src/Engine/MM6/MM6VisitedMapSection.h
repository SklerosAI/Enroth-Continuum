#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"
#include "MM6VisitedMapCache.h"

namespace mm6 {

enum class ActiveMapBranch {
    OutdoorV5,
    IndoorV6,
};

/**
 * An active-map save, outdoor v5 or indoor v6, with the visited maps of the session next to it.
 */
struct SessionWithVisitedMaps {
    ActiveMapBranch branch = ActiveMapBranch::OutdoorV5;
    std::string activeMap;
    std::vector<std::uint8_t> active; // Canonical bytes of the active branch, without the visited maps.
    VisitedMapCache visited;
};

/**
 * Encodes outdoor and indoor visited maps into one `VIST` section. Its payload version 2 is distinct from the
 * outdoor-only version 1 of the v6 outdoor branch, so neither decoder accepts the other's payload.
 *
 * @param visited                       Visited maps, may be empty.
 * @param activeMap                     Map the party is on, it must not have a visited owner.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Required `VIST` section.
 */
SaveSection encodeVisitedMapSection(const VisitedMapCache &visited, std::string_view activeMap, GameTitle title);

/**
 * @param container                     Container holding a version 2 `VIST` section.
 * @param activeMap                     Map the party is on, it must not have a visited owner.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Structurally validated visited maps. Source agreement of each owner is
 *                                      checked separately, before the owner is used.
 */
VisitedMapCache decodeVisitedMapSection(const SaveContainer &container, std::string_view activeMap, GameTitle title);

/**
 * Appends the visited maps to an active-map save. The active save is fully decoded first.
 *
 * @param active                        Encoded outdoor v5 or indoor v6 save.
 * @param visited                       Visited maps of the session.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Version 6 container with the version 2 `VIST` section last.
 */
std::vector<std::uint8_t> encodeSessionWithVisitedMaps(std::span<const std::uint8_t> active,
                                                       const VisitedMapCache &visited, GameTitle title);

/**
 * Splits and validates a container written by `encodeSessionWithVisitedMaps`. Throws on any malformed,
 * non-canonical or colliding owner.
 *
 * @param bytes                         Encoded session.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Active branch and its canonical bytes, and the visited maps.
 */
SessionWithVisitedMaps decodeSessionWithVisitedMaps(std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
