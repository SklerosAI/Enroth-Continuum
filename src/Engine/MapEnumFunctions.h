#pragma once

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
