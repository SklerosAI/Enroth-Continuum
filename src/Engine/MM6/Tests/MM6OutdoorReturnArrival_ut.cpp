#include <limits>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorReturnArrival.h"

UNIT_TEST(MM6OutdoorReturnArrival, WrapsIntoDetachedPartyAtTargetTerrain) {
    auto base = std::make_unique<OutdoorLocation_MM7>();
    base->heightMap.fill(5);
    auto live = std::make_unique<Party>();
    live->pos = Vec3f(22529, 77, 900);
    live->velocity = Vec3f(3, 4, -5);
    live->_viewPitch = 42;
    live->uNumGold = 1234;
    live->radius = 37;

    const auto stage = mm6::stageDetachedOutdoorReturnArrival(
        GameTitle::MM6, "outc2.odm", "outd2.odm", *base, *live);
    ASSERT_EQ(stage.refusal, mm6::OutdoorReturnArrivalRefusal::None);
    ASSERT_NE(stage.detachedParty, nullptr);
    ASSERT_TRUE(stage.placement.has_value());
    EXPECT_EQ(stage.detachedParty->pos, Vec3f(-22527, 77, 160));
    EXPECT_EQ(stage.placement->pos, stage.detachedParty->pos);
    EXPECT_EQ(stage.detachedParty->velocity, Vec3f(0, 0, 0));
    EXPECT_EQ(stage.detachedParty->uFallStartZ, 160);
    EXPECT_EQ(stage.detachedParty->_viewPitch, 42);
    EXPECT_EQ(stage.detachedParty->uNumGold, 1234);
    EXPECT_EQ(live->pos, Vec3f(22529, 77, 900));
    EXPECT_EQ(live->velocity, Vec3f(3, 4, -5));
    EXPECT_FALSE(stage.canPublish());
    EXPECT_FALSE(stage.canWriteCheckpoint());
}

UNIT_TEST(MM6OutdoorReturnArrival, UsesNativeSlopeTriangles) {
    auto base = std::make_unique<OutdoorLocation_MM7>();
    base->heightMap.fill(0);
    // Cell (64, 63) surrounds the origin; distinguish both diagonals.
    base->heightMap[63 * 128 + 64] = 1;
    base->heightMap[63 * 128 + 65] = 2;
    base->heightMap[64 * 128 + 64] = 3;
    base->heightMap[64 * 128 + 65] = 4;
    EXPECT_EQ(mm6::outdoorReturnTerrainHeight(*base, 128, 384), 56);
    EXPECT_EQ(mm6::outdoorReturnTerrainHeight(*base, 384, 128), 104);
}

UNIT_TEST(MM6OutdoorReturnArrival, RefusesInvalidRouteAndModelFloorWithoutMutation) {
    auto base = std::make_unique<OutdoorLocation_MM7>();
    base->heightMap.fill(1);
    auto live = std::make_unique<Party>();
    live->pos = Vec3f(22529, 77, 900);
    live->uNumGold = 1234;
    live->radius = 37;
    const auto refused = [&](GameTitle title, std::string_view source,
                             std::string_view target) {
        auto stage = mm6::stageDetachedOutdoorReturnArrival(
            title, source, target, *base, *live);
        EXPECT_EQ(stage.detachedParty, nullptr);
        EXPECT_FALSE(stage.placement.has_value());
        EXPECT_EQ(live->pos, Vec3f(22529, 77, 900));
        EXPECT_EQ(live->uNumGold, 1234);
        return stage.refusal;
    };
    EXPECT_EQ(refused(GameTitle::MM7, "outc2.odm", "outd2.odm"),
              mm6::OutdoorReturnArrivalRefusal::WrongTitle);
    EXPECT_EQ(refused(GameTitle::MM6, "bad.odm", "outd2.odm"),
              mm6::OutdoorReturnArrivalRefusal::InvalidRoute);
    EXPECT_EQ(refused(GameTitle::MM6, "outc2.odm", "oute2.odm"),
              mm6::OutdoorReturnArrivalRefusal::WrongTarget);
    base->models.emplace_back().minX = -22540;
    auto &model = base->models.back();
    model.maxX = -22510;
    model.minY = 50;
    model.maxY = 100;
    EXPECT_EQ(refused(GameTitle::MM6, "outc2.odm", "outd2.odm"),
              mm6::OutdoorReturnArrivalRefusal::ModelFloorUnverified);
    base->models.clear();
    live->pos.x = std::numeric_limits<float>::quiet_NaN();
    auto invalid = mm6::stageDetachedOutdoorReturnArrival(
        GameTitle::MM6, "outc2.odm", "outd2.odm", *base, *live);
    EXPECT_EQ(invalid.refusal, mm6::OutdoorReturnArrivalRefusal::InvalidRoute);
    EXPECT_EQ(invalid.detachedParty, nullptr);
}
