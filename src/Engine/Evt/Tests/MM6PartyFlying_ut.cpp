#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6PartyFlying.h"

UNIT_TEST(MM6PartyFlying, RequiresFlagAndActiveBuff) {
    EXPECT_FALSE(mm6::comparePartyFlying(222, 0, false, false));
    EXPECT_FALSE(mm6::comparePartyFlying(222, 0, true, false));
    EXPECT_FALSE(mm6::comparePartyFlying(222, 0, false, true));
    EXPECT_TRUE(mm6::comparePartyFlying(222, 0, true, true));
}

UNIT_TEST(MM6PartyFlying, OriginalComparisonIgnoresNumericOperand) {
    EXPECT_TRUE(mm6::comparePartyFlying(222, 1, true, true));
    EXPECT_TRUE(mm6::comparePartyFlying(222, -1, true, true));
    EXPECT_FALSE(mm6::comparePartyFlying(222, 99, false, true));
    EXPECT_THROW(mm6::comparePartyFlying(226, 0, true, true), std::runtime_error);
}
