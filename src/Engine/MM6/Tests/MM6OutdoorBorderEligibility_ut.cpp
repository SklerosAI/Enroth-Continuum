#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorBorderEligibility.h"

UNIT_TEST(MM6OutdoorBorderEligibility, RefusesEachOriginalOfferFlag) {
    EXPECT_TRUE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, {}));
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, PARTY_FLAG_WATER_DAMAGE));
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, PARTY_FLAG_AIRBORNE));
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, PARTY_FLAG_STANDING_ON_WATER));
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6,
        PARTY_FLAG_WATER_DAMAGE | PARTY_FLAG_AIRBORNE | PARTY_FLAG_STANDING_ON_WATER));
}

UNIT_TEST(MM6OutdoorBorderEligibility, BurningAndUnrelatedFlagsDoNotBlockOriginalOfferMask) {
    EXPECT_TRUE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, PARTY_FLAG_BURNING));
    EXPECT_TRUE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, PARTY_FLAG_ALERT_RED));
    EXPECT_TRUE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6,
        PARTY_FLAG_BURNING | PARTY_FLAG_ALERT_RED | PARTY_FLAG_LANDING));
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6,
        PARTY_FLAG_BURNING | PARTY_FLAG_WATER_DAMAGE));
}

UNIT_TEST(MM6OutdoorBorderEligibility, OfferRefusesFlyingLikeTheConfirmation) {
    EXPECT_TRUE(mm6::borderConfirmFlyingStatePermits(GameTitle::MM6, 0));
    EXPECT_FALSE(mm6::borderConfirmFlyingStatePermits(GameTitle::MM6, 1));
    EXPECT_FALSE(mm6::borderConfirmFlyingStatePermits(GameTitle::MM6, 2));

    EXPECT_TRUE(mm6::borderOfferPartyStatePermits(GameTitle::MM6, {}, 0));
    EXPECT_FALSE(mm6::borderOfferPartyStatePermits(GameTitle::MM6, {}, 1));
    EXPECT_FALSE(mm6::borderOfferPartyStatePermits(GameTitle::MM6, {}, 2));
    EXPECT_FALSE(mm6::borderOfferPartyStatePermits(GameTitle::MM6, PARTY_FLAG_AIRBORNE, 0));
    EXPECT_TRUE(mm6::borderOfferPartyStatePermits(GameTitle::MM6, PARTY_FLAG_BURNING, 0));

    // The original offer reads only the flags, so a low flight passes it.
    EXPECT_TRUE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM6, {}));
}

UNIT_TEST(MM6OutdoorBorderEligibility, ForeignTitleAndInputsRemainUnchanged) {
    const PartyFlags flags = PARTY_FLAG_BURNING | PARTY_FLAG_AIRBORNE;
    const unsigned int flying = 1;
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM7, {}));
    EXPECT_FALSE(mm6::borderOfferPartyFlagsPermit(GameTitle::MM7, flags));
    EXPECT_FALSE(mm6::borderConfirmFlyingStatePermits(GameTitle::MM7, 0));
    EXPECT_FALSE(mm6::borderConfirmFlyingStatePermits(GameTitle::MM7, flying));
    EXPECT_FALSE(mm6::borderOfferPartyStatePermits(GameTitle::MM7, {}, 0));
    EXPECT_TRUE(flags == (PARTY_FLAG_BURNING | PARTY_FLAG_AIRBORNE));
    EXPECT_EQ(flying, 1);
}
