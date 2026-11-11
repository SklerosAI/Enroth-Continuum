#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorBorderTravel.h"

UNIT_TEST(MM6OutdoorBorderTravel, ResolvesFourEdgesAndPreservesOffset) {
    const mm6::BorderTravelHirelings hirelings;
    const auto west = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", -22529, 17, hirelings);
    const auto east = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 22529, 17, hirelings);
    const auto south = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 17, -22529, hirelings);
    const auto north = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 17, 22529, hirelings);

    ASSERT_TRUE(west.candidate.has_value());
    ASSERT_TRUE(east.candidate.has_value());
    ASSERT_TRUE(south.candidate.has_value());
    ASSERT_TRUE(north.candidate.has_value());
    EXPECT_EQ(west.candidate->destinationMap, "outb2.odm");
    EXPECT_EQ(east.candidate->destinationMap, "outd2.odm");
    EXPECT_EQ(south.candidate->destinationMap, "outc3.odm");
    EXPECT_EQ(north.candidate->destinationMap, "outc1.odm");
    EXPECT_EQ(west.candidate->arrivalX, 22527);
    EXPECT_EQ(east.candidate->arrivalX, -22527);
    EXPECT_EQ(south.candidate->arrivalY, 22527);
    EXPECT_EQ(north.candidate->arrivalY, -22527);
    EXPECT_EQ(west.candidate->arrivalY, 17);
    EXPECT_EQ(north.candidate->arrivalX, 17);
    EXPECT_EQ(west.candidate->travelDays, 5);
    EXPECT_TRUE(west.refusal == mm6::BorderTravelRefusal::None);
}

UNIT_TEST(MM6OutdoorBorderTravel, MovesBothAxesAtCorners) {
    const mm6::BorderTravelHirelings hirelings;
    const auto northwest = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", -22529, 22529, hirelings);
    const auto northeast = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 22529, 22529, hirelings);
    const auto southwest = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", -22529, -22529, hirelings);
    const auto southeast = mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 22529, -22529, hirelings);

    ASSERT_TRUE(northwest.candidate.has_value());
    ASSERT_TRUE(northeast.candidate.has_value());
    ASSERT_TRUE(southwest.candidate.has_value());
    ASSERT_TRUE(southeast.candidate.has_value());
    EXPECT_EQ(northwest.candidate->destinationMap, "outb1.odm");
    EXPECT_EQ(northeast.candidate->destinationMap, "outd1.odm");
    EXPECT_EQ(southwest.candidate->destinationMap, "outb3.odm");
    EXPECT_EQ(southeast.candidate->destinationMap, "outd3.odm");
    EXPECT_EQ(northwest.candidate->arrivalX, 22527);
    EXPECT_EQ(northwest.candidate->arrivalY, -22527);
    EXPECT_TRUE(northwest.candidate->crossedX);
    EXPECT_TRUE(northwest.candidate->crossedY);
    EXPECT_TRUE(northwest.refusal == mm6::BorderTravelRefusal::None);
}

UNIT_TEST(MM6OutdoorBorderTravel, RefusesInvalidBordersAndCoordinates) {
    const mm6::BorderTravelHirelings hirelings;
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM7, "outc2.odm", 22529, 0, hirelings).refusal ==
                mm6::BorderTravelRefusal::WrongTitle);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "OUTC2.ODM", 22529, 0, hirelings).refusal ==
                mm6::BorderTravelRefusal::InvalidSourceMap);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 22528, -22528, hirelings).refusal ==
                mm6::BorderTravelRefusal::NoBorderCrossing);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outa1.odm", -22529, 0, hirelings).refusal ==
                mm6::BorderTravelRefusal::OutsideGrid);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "oute3.odm", 22529, -22529, hirelings).refusal ==
                mm6::BorderTravelRefusal::OutsideGrid);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm",
                                               std::numeric_limits<float>::infinity(), 0, hirelings).refusal ==
                mm6::BorderTravelRefusal::InvalidCoordinates);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm",
                                               std::numeric_limits<float>::quiet_NaN(), 0, hirelings).refusal ==
                mm6::BorderTravelRefusal::InvalidCoordinates);
    EXPECT_TRUE(mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", 70000, 0, hirelings).refusal ==
                mm6::BorderTravelRefusal::ArrivalOutsideBounds);
}

UNIT_TEST(MM6OutdoorBorderTravel, AppliesBoundedHirelingReductionsWithoutMutation) {
    EXPECT_EQ(mm6::borderTravelDays({}), 5);
    EXPECT_EQ(mm6::borderTravelDays({.guide = true}), 4);
    EXPECT_EQ(mm6::borderTravelDays({.tracker = true}), 3);
    EXPECT_EQ(mm6::borderTravelDays({.pathfinder = true}), 2);
    EXPECT_EQ(mm6::borderTravelDays({.explorer = true}), 4);
    const mm6::BorderTravelHirelings all{true, true, true, true};
    EXPECT_EQ(mm6::borderTravelDays(all), 1);

    const std::string source = "outc2.odm";
    const float x = 22529;
    const float y = -22529;
    const auto assessment = mm6::assessOutdoorBorderTravel(GameTitle::MM6, source, x, y, all);
    ASSERT_TRUE(assessment.candidate.has_value());
    EXPECT_EQ(assessment.candidate->travelDays, 1);
    EXPECT_EQ(source, "outc2.odm");
    EXPECT_EQ(x, 22529);
    EXPECT_EQ(y, -22529);
    EXPECT_TRUE(all.guide && all.tracker && all.pathfinder && all.explorer);
}

UNIT_TEST(MM6OutdoorBorderTravel, HirelingsAreGuideTrackerPathfinderAndExplorer) {
    // Border travel is shortened by the MM6 Guide, Tracker, Pathfinder and Explorer, rows 5, 6, 7 and 44 of npcprof.txt.
    std::vector<int> asked;
    const mm6::BorderTravelHirelings none = mm6::borderTravelHirelings([&](NpcProfession profession) {
        asked.push_back(std::to_underlying(profession));
        return false;
    });
    EXPECT_EQ(asked, (std::vector<int>{5, 6, 7, 44}));
    EXPECT_EQ(mm6::borderTravelDays(none), 5);

    const auto hiring = [](NpcProfession hired) {
        return mm6::borderTravelHirelings([hired](NpcProfession profession) { return profession == hired; });
    };
    EXPECT_EQ(mm6::borderTravelDays(hiring(NPC_PROFESSION_GUIDE)), 4);
    EXPECT_EQ(mm6::borderTravelDays(hiring(NPC_PROFESSION_TRACKER)), 3);
    EXPECT_EQ(mm6::borderTravelDays(hiring(NPC_PROFESSION_PATHFINDER)), 2);
    EXPECT_EQ(mm6::borderTravelDays(hiring(NPC_PROFESSION_EXPLORER)), 4);
    EXPECT_EQ(mm6::borderTravelDays(hiring(NPC_PROFESSION_SAILOR)), 5);
}
