#pragma once

#include "Utility/Segment.h"

#include "GameTitle.h"
#include "MapEnums.h"

/**
 * Is map an outdoor map?
 */
inline bool isMapOutdoor(MapId mapid) {
    return mapid >= MAP_EMERALD_ISLAND && mapid <= MAP_SHOALS && mapid != MAP_PIT && mapid != MAP_CELESTE;
}

/**
 * Is map an indoor map?
 */
inline bool isMapIndoor(MapId mapid) {
    return (mapid >= MAP_DRAGON_CAVES && mapid <= MAP_ARENA) || mapid == MAP_PIT || mapid == MAP_CELESTE;
}

inline constexpr MapId MM6_MAP_LAST_OUTDOOR = static_cast<MapId>(15); // MM6 mapstats.txt lists its 15 outdoor maps first.
inline constexpr MapId MM6_MAP_LAST = static_cast<MapId>(67); // Then its 52 indoor maps.

/**
 * @param title                         Game title that numbered `mapid`.
 * @param mapid                         Map to check.
 * @return                              Whether the map is an outdoor map. MM6 map IDs don't follow the MM7 enum.
 */
inline bool isMapOutdoor(GameTitle title, MapId mapid) {
    if (title == GameTitle::MM6)
        return mapid >= MAP_FIRST && mapid <= MM6_MAP_LAST_OUTDOOR;
    return isMapOutdoor(mapid);
}

/**
 * @param title                         Game title that numbered `mapid`.
 * @param mapid                         Map to check.
 * @return                              Whether the map is an indoor map. MM6 map IDs don't follow the MM7 enum.
 */
inline bool isMapIndoor(GameTitle title, MapId mapid) {
    if (title == GameTitle::MM6)
        return mapid > MM6_MAP_LAST_OUTDOOR && mapid <= MM6_MAP_LAST;
    return isMapIndoor(mapid);
}

/**
 * Is map an outdoor underwater map (requires wetsuit etc.)?
 */
inline bool isMapUnderwater(MapId mapid) {
    return mapid == MAP_SHOALS;
}

/**
 * Is hirelings interactions are forbidden on this map?
 */
inline bool isHirelingsBlockedOnMap(MapId mapid) {
    return (mapid == MAP_SHOALS) || (mapid == MAP_LINCOLN);
}

inline bool isHirelingsBlockedOnMap(GameTitle title, MapId mapid) {
    // MM6 map IDs reuse the MM7 enum values; its first outdoor map is 15,
    // which is MAP_SHOALS only in MM7.
    return title == GameTitle::MM6 ? false : isHirelingsBlockedOnMap(mapid);
}

inline Segment<MapId> allMaps() {
    return {MAP_FIRST, MAP_LAST};
}
