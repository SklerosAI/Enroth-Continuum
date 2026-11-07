#include <limits>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorBorderPrecommit.h"

UNIT_TEST(MM6OutdoorBorderPrecommit, ClampsOnlyDetachedOldMapParty) {
    auto live = std::make_unique<Party>();
    live->pos = Vec3f(22529, 77, 300);
    live->uNumGold = 1234;

    const auto stage = mm6::stageBorderOldMapParty(
        GameTitle::MM6, "outc2.odm", *live, {});
    ASSERT_TRUE(stage.destination.has_value());
    ASSERT_NE(stage.oldMapParty, nullptr);
    EXPECT_EQ(stage.destination->destinationMap, "outd2.odm");
    EXPECT_EQ(stage.destination->arrivalX, -22527);
    EXPECT_EQ(stage.oldMapParty->pos.x, mm6::MM6_OUTDOOR_BORDER_EDGE);
    EXPECT_EQ(stage.oldMapParty->pos.y, 77);
    EXPECT_EQ(stage.oldMapParty->pos.z, 300);
    EXPECT_EQ(stage.oldMapParty->uNumGold, 1234);
    EXPECT_EQ(live->pos.x, 22529);
    EXPECT_EQ(live->uNumGold, 1234);
    EXPECT_EQ(stage.refusal, mm6::BorderPrecommitRefusal::MissingWorldAndSourceProof);
    EXPECT_FALSE(stage.canWriteCheckpoint());
}

UNIT_TEST(MM6OutdoorBorderPrecommit, RefusesWrongTitleAndUnprovedPartyState) {
    auto live = std::make_unique<Party>();
    live->pos = Vec3f(-22529, 0, 100);
    EXPECT_EQ(mm6::stageBorderOldMapParty(GameTitle::MM7, "outc2.odm", *live, {}).refusal,
              mm6::BorderPrecommitRefusal::WrongTitle);

    live->uFlags |= PARTY_FLAG_AIRBORNE;
    auto flags = mm6::stageBorderOldMapParty(GameTitle::MM6, "outc2.odm", *live, {});
    EXPECT_EQ(flags.refusal, mm6::BorderPrecommitRefusal::PartyState);
    EXPECT_EQ(flags.oldMapParty, nullptr);
    live->uFlags &= ~PARTY_FLAG_AIRBORNE;

    live->bFlying = 1;
    EXPECT_EQ(mm6::stageBorderOldMapParty(GameTitle::MM6, "outc2.odm", *live, {}).refusal,
              mm6::BorderPrecommitRefusal::PartyState);
    live->bFlying = 0;
    live->pos.z = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(mm6::stageBorderOldMapParty(GameTitle::MM6, "outc2.odm", *live, {}).refusal,
              mm6::BorderPrecommitRefusal::PartyState);
    EXPECT_EQ(live->pos.x, -22529);
}

UNIT_TEST(MM6OutdoorBorderPrecommit, RefusesUnprovedRouteAndCorner) {
    auto live = std::make_unique<Party>();
    live->pos = Vec3f(22529, 17, 100);
    EXPECT_EQ(mm6::stageBorderOldMapParty(GameTitle::MM6, "bad.odm", *live, {}).refusal,
              mm6::BorderPrecommitRefusal::NoCardinalRoute);
    live->pos.x = 22528;
    EXPECT_EQ(mm6::stageBorderOldMapParty(GameTitle::MM6, "outc2.odm", *live, {}).refusal,
              mm6::BorderPrecommitRefusal::NoCardinalRoute);
    live->pos = Vec3f(22529, 22529, 100);
    const auto corner = mm6::stageBorderOldMapParty(GameTitle::MM6, "outc2.odm", *live, {});
    EXPECT_EQ(corner.refusal, mm6::BorderPrecommitRefusal::CornerUnverified);
    EXPECT_EQ(corner.oldMapParty, nullptr);
    EXPECT_FALSE(corner.canWriteCheckpoint());
    EXPECT_EQ(live->pos.x, 22529);
    EXPECT_EQ(live->pos.y, 22529);
}
