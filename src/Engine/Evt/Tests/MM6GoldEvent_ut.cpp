#include <limits>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6GoldEvent.h"

UNIT_TEST(MM6GoldEvent, CompareAddAndPayHaveBoundedPartyValues) {
    EXPECT_TRUE(mm6::hasEventGold(300, 300));
    EXPECT_FALSE(mm6::hasEventGold(299, 300));
    EXPECT_EQ(mm6::goldAfterEventAdd(300, 500), 800);
    EXPECT_EQ(mm6::goldAfterEventAdd(0, 250000), 250000);
    EXPECT_EQ(mm6::goldAfterEventSubtract(800, 300), 500);
    EXPECT_FALSE(mm6::goldAfterEventSubtract(299, 300).has_value());
    EXPECT_EQ(mm6::goldAfterEventSubtract(0, 0), 0);
}

UNIT_TEST(MM6GoldEvent, InvalidAndOverflowRejectBeforeMutation) {
    EXPECT_THROW(mm6::hasEventGold(-1, 0), std::runtime_error);
    EXPECT_THROW(mm6::hasEventGold(0, -1), std::runtime_error);
    EXPECT_THROW(mm6::goldAfterEventAdd(1, -1), std::runtime_error);
    EXPECT_THROW(mm6::goldAfterEventAdd(std::numeric_limits<int>::max(), 1), std::runtime_error);
    EXPECT_THROW(mm6::goldAfterEventSubtract(1, -1), std::runtime_error);
}
