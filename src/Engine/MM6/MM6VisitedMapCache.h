#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6VisitedIndoorRuntimeOwner.h"
#include "MM6VisitedOutdoorRuntimeOwner.h"

namespace mm6 {

using VisitedMapOwner = std::variant<VisitedOutdoorRuntimeOwner, VisitedIndoorRuntimeOwner>;

/**
 * @param owner                         Visited map owner.
 * @return                              Map file name the owner belongs to.
 */
[[nodiscard]] std::string_view visitedMapName(const VisitedMapOwner &owner);

/**
 * Runtime owners of every map the party has left in this session, at most one per map, sorted by map name.
 * A map that is parked again replaces its previous owner. Nothing here is published to the live world.
 */
class VisitedMapCache {
 public:
    /** Every MM6 map but the active one. */
    static constexpr std::size_t MAX_MAPS = 66;

    /**
     * Validates the departing map's owner structurally and stores it. Throws without changing the cache if the owner
     * fails validation or the cache is full.
     *
     * @param owner                     Owner of the map the party is leaving.
     * @param title                     Game title, only MM6 is accepted.
     */
    void park(VisitedMapOwner owner, GameTitle title);

    /**
     * @param mapName                   Map file name.
     * @return                          Owner of `mapName`, or `nullptr` if the map wasn't visited.
     */
    [[nodiscard]] const VisitedMapOwner *find(std::string_view mapName) const;

    /**
     * Removes and returns the owner of a map the party is returning to.
     *
     * @param mapName                   Map file name.
     * @return                          Owner of `mapName`, or `std::nullopt` if the map wasn't visited.
     */
    [[nodiscard]] std::optional<VisitedMapOwner> take(std::string_view mapName);

    /**
     * @param activeMap                 Map the party is on.
     * @throws std::runtime_error       If `activeMap` also has a visited owner.
     */
    void requireSeparateFrom(std::string_view activeMap) const;

    [[nodiscard]] std::span<const VisitedMapOwner> entries() const { return _entries; }
    [[nodiscard]] std::size_t size() const { return _entries.size(); }
    [[nodiscard]] bool empty() const { return _entries.empty(); }

 private:
    std::vector<VisitedMapOwner> _entries;
};

} // namespace mm6
