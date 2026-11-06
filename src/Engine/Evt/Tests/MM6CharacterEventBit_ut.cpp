#include <array>
#include <cstdint>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6CharacterEventBit.h"

#include "Library/Snapshots/CommonSnapshots.h"
#include "Library/Snapshots/SnapshotTags.h"

UNIT_TEST(MM6CharacterEventBit, SetAndCompareAreIdempotentAndBounded) {
    IndexedBitset<1, 512> bits;
    mm6::CharacterEventBitState state(bits);
    EXPECT_FALSE(state.compare(213, 0));
    EXPECT_FALSE(state.compare(213, 511));
    state.set(213, 0);
    state.set(213, 511);
    state.set(213, 511);
    EXPECT_TRUE(state.compare(213, 0));
    EXPECT_TRUE(state.compare(213, 511));
    EXPECT_TRUE(bits.test(1));
    EXPECT_TRUE(bits.test(512));
    EXPECT_FALSE(state.compare(213, 41));

    std::array<std::uint8_t, 64> serialized{};
    snapshot(bits, &serialized, tags::reverseBits);
    EXPECT_EQ(serialized.front(), 0x80);
    EXPECT_EQ(serialized.back(), 0x01);
}

UNIT_TEST(MM6CharacterEventBit, InvalidInputRejectsBeforeMutation) {
    IndexedBitset<1, 512> bits;
    mm6::CharacterEventBitState state(bits);
    EXPECT_THROW(state.set(212, 1), std::runtime_error);
    EXPECT_THROW(state.set(213, -1), std::runtime_error);
    EXPECT_THROW(state.set(213, 512), std::runtime_error);
    EXPECT_THROW(state.compare(214, 1), std::runtime_error);
    EXPECT_FALSE(state.compare(213, 1));
    EXPECT_FALSE(state.compare(213, 511));
}
