#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6AutonoteState.h"

UNIT_TEST(MM6AutonoteState, BoundedPartyBitsAndIdempotentWrites) {
    IndexedBitset<1, 208> bits;
    bits.set(129); // Outside the shipped MM6 autonote table; must survive.
    MM6AutonoteState state(bits);
    EXPECT_FALSE(state.compare(205, 1));
    EXPECT_TRUE(state.set(205, 1));
    EXPECT_TRUE(state.compare(205, 1));
    EXPECT_FALSE(state.add(205, 1));
    EXPECT_TRUE(state.add(205, 128));
    EXPECT_TRUE(state.compare(205, 128));
    EXPECT_TRUE(state.subtract(205, 1));
    EXPECT_FALSE(state.compare(205, 1));
    EXPECT_FALSE(state.subtract(205, 1));
    EXPECT_TRUE(bits.test(129));
}

UNIT_TEST(MM6AutonoteState, InvalidOperandsRejectBeforeMutation) {
    IndexedBitset<1, 208> bits;
    bits.set(40);
    MM6AutonoteState state(bits);
    EXPECT_THROW(state.set(204, 41), std::runtime_error);
    EXPECT_THROW(state.add(205, 0), std::runtime_error);
    EXPECT_THROW(state.subtract(205, 129), std::runtime_error);
    EXPECT_THROW((void) state.compare(205, -1), std::runtime_error);
    EXPECT_TRUE(bits.test(40));
    EXPECT_FALSE(bits.test(41));
    EXPECT_FALSE(bits.test(128));
}
