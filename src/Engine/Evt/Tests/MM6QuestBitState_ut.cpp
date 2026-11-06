#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6QuestBitState.h"

UNIT_TEST(MM6QuestBitState, PartyBitsAreIdempotentAndBounded) {
    IndexedBitset<QBIT_FIRST, QBIT_LAST> bits;
    MM6QuestBitState state(bits);
    EXPECT_FALSE(state.compare(16, 1));
    EXPECT_TRUE(state.add(16, 1));
    EXPECT_TRUE(state.set(16, 512));
    EXPECT_TRUE(state.compare(16, 1));
    EXPECT_TRUE(state.compare(16, 512));
    EXPECT_FALSE(state.add(16, 1));
    EXPECT_FALSE(state.set(16, 512));
    EXPECT_TRUE(state.subtract(16, 1));
    EXPECT_FALSE(state.subtract(16, 1));
    EXPECT_FALSE(state.compare(16, 1));
    EXPECT_TRUE(state.compare(16, 512));
}

UNIT_TEST(MM6QuestBitState, InvalidOperandsRejectBeforeMutation) {
    IndexedBitset<QBIT_FIRST, QBIT_LAST> bits;
    bits.set(static_cast<QuestBit>(42));
    MM6QuestBitState state(bits);
    EXPECT_THROW((void) state.compare(17, 42), std::runtime_error);
    EXPECT_THROW((void) state.compare(16, 0), std::runtime_error);
    EXPECT_THROW(state.add(16, 513), std::runtime_error);
    EXPECT_THROW(state.subtract(16, -1), std::runtime_error);
    EXPECT_THROW(state.set(17, 43), std::runtime_error);
    EXPECT_TRUE(bits.test(static_cast<QuestBit>(42)));
    EXPECT_FALSE(bits.test(static_cast<QuestBit>(43)));
}
