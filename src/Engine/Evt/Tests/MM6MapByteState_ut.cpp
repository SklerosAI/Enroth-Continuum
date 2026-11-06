#include <cstdint>
#include <stdexcept>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6MapByteState.h"

UNIT_TEST(MM6MapByteState, ActiveEndpointsAndNonzeroOperations) {
    std::vector<std::uint8_t> delta(300, 0xA5);
    MM6MapByteState state(delta);

    state.set(105, 3);
    state.add(105, 4);
    EXPECT_FALSE(state.compare(105, 8));
    EXPECT_TRUE(state.compare(105, 7));
    state.subtract(105, 2);
    EXPECT_TRUE(state.compare(105, 5));
    state.set(175, 30);
    EXPECT_TRUE(state.compare(175, 30));

    // 300-byte fixture: persistent bytes at 44..243, tail at 244..299.
    EXPECT_EQ(delta[44], 5);
    EXPECT_EQ(delta[114], 30);
    for (size_t i = 0; i < delta.size(); ++i)
        if (i != 44 && i != 114)
            EXPECT_EQ(delta[i], 0xA5) << i;
}

UNIT_TEST(MM6MapByteState, RejectsUnknownSlotsAndUnprovedArithmetic) {
    std::vector<std::uint8_t> delta(256, 0);
    MM6MapByteState state(delta);
    EXPECT_THROW(state.set(104, 1), std::runtime_error);
    EXPECT_THROW(state.set(176, 1), std::runtime_error);
    EXPECT_THROW(state.compare(204, 0), std::runtime_error);
    EXPECT_THROW(state.set(105, -1), std::runtime_error);
    EXPECT_THROW(state.add(105, 256), std::runtime_error);
    EXPECT_THROW(state.subtract(105, 1), std::runtime_error);

    state.set(105, 255);
    EXPECT_THROW(state.add(105, 1), std::runtime_error);
    EXPECT_EQ(delta[0], 255);
    EXPECT_EQ(delta[71], 0);
    EXPECT_EQ(delta[199], 0);
    EXPECT_EQ(delta[200], 0);
    EXPECT_THROW(MM6MapByteState(std::span<std::uint8_t>(delta).first(255)), std::runtime_error);
}
