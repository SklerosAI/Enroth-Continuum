#include <exception>
#include <string>
#include <variant>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6VisitedMapCache.h"
#include "Engine/MM6/Tests/MM6VisitedOwnerFixtures.h"

namespace fixtures = mm6::testing;

static mm6::VisitedIndoorRuntimeOwner visitedIndoor(std::string_view mapName) {
    DecorationTable decorations;
    const auto staged = fixtures::fixtureStagedIndoorSave(fixtures::fixtureIndoorSave(), decorations);
    const auto owner = mm6::stageVisitedIndoorRuntimeOwner(staged, "d0.blv", decorations, fixtures::fixtureNoFaces,
                                                           GameTitle::MM6);
    return fixtures::renamedVisitedIndoor(owner, mapName);
}

UNIT_TEST(MM6VisitedMapCache, KeepsOneSortedOwnerPerMap) {
    // Every map the party left keeps its own owner, and leaving it again replaces the older one.
    mm6::VisitedMapCache cache;
    cache.park(fixtures::fixtureVisitedOutdoor("oute2.odm"), GameTitle::MM6);
    cache.park(visitedIndoor("d1.blv"), GameTitle::MM6);
    cache.park(fixtures::fixtureVisitedOutdoor("outa1.odm"), GameTitle::MM6);
    ASSERT_EQ(cache.size(), 3u);
    EXPECT_EQ(mm6::visitedMapName(cache.entries()[0]), "d1.blv");
    EXPECT_EQ(mm6::visitedMapName(cache.entries()[1]), "outa1.odm");
    EXPECT_EQ(mm6::visitedMapName(cache.entries()[2]), "oute2.odm");
    EXPECT_TRUE(std::holds_alternative<mm6::VisitedIndoorRuntimeOwner>(cache.entries()[0]));

    auto again = fixtures::fixtureVisitedOutdoor("oute2.odm");
    again.chest.lcgState = 77;
    cache.park(again, GameTitle::MM6);
    ASSERT_EQ(cache.size(), 3u);
    EXPECT_EQ(std::get<mm6::VisitedOutdoorRuntimeOwner>(*cache.find("oute2.odm")).chest.lcgState, 77u);

    EXPECT_EQ(cache.find("d2.blv"), nullptr);
    EXPECT_NO_THROW(cache.requireSeparateFrom("d2.blv"));
    EXPECT_THROW(cache.requireSeparateFrom("d1.blv"), std::exception);

    const auto taken = cache.take("d1.blv");
    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(mm6::visitedMapName(*taken), "d1.blv");
    EXPECT_EQ(cache.size(), 2u);
    EXPECT_FALSE(cache.take("d1.blv").has_value());
}

UNIT_TEST(MM6VisitedMapCache, RefusesInvalidOwnerWithoutChange) {
    mm6::VisitedMapCache cache;
    cache.park(fixtures::fixtureVisitedOutdoor("oute2.odm"), GameTitle::MM6);

    auto broken = fixtures::fixtureVisitedOutdoor("oute1.odm");
    broken.sprites.mapName = "oute2.odm";
    EXPECT_THROW(cache.park(broken, GameTitle::MM6), std::exception);
    EXPECT_THROW(cache.park(fixtures::fixtureVisitedOutdoor("oute1.odm"), GameTitle::MM7), std::exception);
    auto brokenIndoor = visitedIndoor("d1.blv");
    brokenIndoor.chest.activeDeltaName = "d2.dlv";
    EXPECT_THROW(cache.park(brokenIndoor, GameTitle::MM6), std::exception);
    ASSERT_EQ(cache.size(), 1u);
    EXPECT_EQ(mm6::visitedMapName(cache.entries()[0]), "oute2.odm");
}

UNIT_TEST(MM6VisitedMapCache, RefusesMoreThanEveryOtherMap) {
    const auto indoor = visitedIndoor("d0.blv");
    mm6::VisitedMapCache cache;
    for (std::size_t i = 0; i < mm6::VisitedMapCache::MAX_MAPS; ++i)
        cache.park(fixtures::renamedVisitedIndoor(indoor, "d" + std::to_string(i) + ".blv"), GameTitle::MM6);
    EXPECT_THROW(cache.park(fixtures::fixtureVisitedOutdoor("oute2.odm"), GameTitle::MM6), std::exception);
    EXPECT_NO_THROW(cache.park(fixtures::renamedVisitedIndoor(indoor, "d0.blv"), GameTitle::MM6));
    EXPECT_EQ(cache.size(), mm6::VisitedMapCache::MAX_MAPS);
}
