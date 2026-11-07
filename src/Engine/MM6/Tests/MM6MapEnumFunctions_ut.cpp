#include "Testing/Unit/UnitTest.h"

#include "Engine/MapEnumFunctions.h"

UNIT_TEST(MM6MapEnumFunctions, ClassifiesEveryShippedMapId) {
    // The MM7 ranges classified MM6 outdoor maps 7 and 8 as indoor.
    int outdoor = 0;
    int indoor = 0;
    for (int id = 0; id <= static_cast<int>(MAP_LAST); ++id) {
        const MapId map = static_cast<MapId>(id);
        const bool isOutdoor = isMapOutdoor(GameTitle::MM6, map);
        const bool isIndoor = isMapIndoor(GameTitle::MM6, map);
        EXPECT_FALSE(isOutdoor && isIndoor) << id;
        EXPECT_EQ(isOutdoor, id >= 1 && id <= 15) << id;
        EXPECT_EQ(isIndoor, id >= 16 && id <= 67) << id;
        outdoor += isOutdoor;
        indoor += isIndoor;
    }
    EXPECT_EQ(outdoor, 15);
    EXPECT_EQ(indoor, 52);
    EXPECT_TRUE(isMapOutdoor(GameTitle::MM6, static_cast<MapId>(7)));
    EXPECT_TRUE(isMapOutdoor(GameTitle::MM6, static_cast<MapId>(8)));
    EXPECT_FALSE(isMapIndoor(GameTitle::MM6, static_cast<MapId>(7)));
    EXPECT_FALSE(isMapIndoor(GameTitle::MM6, static_cast<MapId>(8)));
}

UNIT_TEST(MM6MapEnumFunctions, MM7KeepsItsRanges) {
    for (int id = 0; id <= static_cast<int>(MAP_LAST); ++id) {
        const MapId map = static_cast<MapId>(id);
        EXPECT_EQ(isMapOutdoor(GameTitle::MM7, map), isMapOutdoor(map)) << id;
        EXPECT_EQ(isMapIndoor(GameTitle::MM7, map), isMapIndoor(map)) << id;
    }
    EXPECT_TRUE(isMapIndoor(GameTitle::MM7, MAP_CELESTE));
    EXPECT_TRUE(isMapIndoor(GameTitle::MM7, MAP_PIT));
}
