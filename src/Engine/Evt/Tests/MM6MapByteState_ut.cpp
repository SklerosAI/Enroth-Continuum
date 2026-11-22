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

UNIT_TEST(MM6MapByteState, RejectsUnknownSlots) {
    std::vector<std::uint8_t> delta(256, 0);
    MM6MapByteState state(delta);
    EXPECT_THROW(state.set(104, 1), std::runtime_error);
    EXPECT_THROW(state.set(176, 1), std::runtime_error);
    EXPECT_THROW(state.compare(204, 0), std::runtime_error);
    EXPECT_THROW(MM6MapByteState(std::span<std::uint8_t>(delta).first(255)), std::runtime_error);
}

UNIT_TEST(MM6MapByteState, AddStopsAt255) {
    // An add past 255 threw. MM6.exe keeps 255.
    std::vector<std::uint8_t> delta(256, 0);
    MM6MapByteState state(delta);
    state.set(105, 250);
    state.add(105, 6);
    EXPECT_EQ(delta[0], 255);
    state.add(105, 1);
    EXPECT_EQ(delta[0], 255);
    EXPECT_EQ(delta[1], 0);
}

UNIT_TEST(MM6MapByteState, SubtractWrapsBelowZero) {
    // A subtract below 0 threw. MM6.exe wraps around.
    std::vector<std::uint8_t> delta(256, 0);
    MM6MapByteState state(delta);
    state.set(106, 1);
    state.subtract(106, 3);
    EXPECT_EQ(delta[1], 254);
    EXPECT_EQ(delta[0], 0);
    EXPECT_EQ(delta[2], 0);
}

UNIT_TEST(MM6MapByteState, WritesTakeTheLowByteAndCompareTakesTheWholeOperand) {
    // Operands outside 0 to 255 threw. MM6.exe writes with the operand's low byte and compares with all of it.
    std::vector<std::uint8_t> delta(256, 0);
    MM6MapByteState state(delta);
    state.set(105, 0x1FE);
    EXPECT_EQ(delta[0], 0xFE);
    state.set(105, -1);
    EXPECT_EQ(delta[0], 0xFF);
    state.set(105, 10);
    state.add(105, 0x102);
    EXPECT_EQ(delta[0], 12);
    state.add(105, -1);
    EXPECT_EQ(delta[0], 255);
    state.subtract(105, 0x103);
    EXPECT_EQ(delta[0], 252);
    state.subtract(105, -2);
    EXPECT_EQ(delta[0], 254);
    EXPECT_TRUE(state.compare(105, -1000));
    EXPECT_FALSE(state.compare(105, 256));
}
