#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6NativeActorResistanceBytes.h"

UNIT_TEST(MM6NativeActorResistanceBytes, RetainsPerActorOverridesWithoutMutation) {
    constexpr size_t actorSection = 8 + 2 * 88 * 11;
    std::vector<std::uint8_t> bytes(8 + 2 * 88 * 11 + 4 + 2 * 548 + 4 + 4 + 200 + 56);
    bytes[actorSection] = 2;
    const size_t first = actorSection + 4;
    const std::array<std::uint8_t, 6> firstValues = {1, 2, 3, 4, 5, 6};
    const std::array<std::uint8_t, 6> secondValues = {10, 20, 30, 40, 50, 60};
    for (size_t i = 0; i < 6; ++i) {
        bytes[first + 80 + i] = firstValues[i];
        bytes[first + 548 + 80 + i] = secondValues[i];
    }
    const mm6::OutdoorDeltaWire wire = mm6::decodeOutdoorDelta(bytes);
    const auto actual = mm6::decodeNativeActorResistanceBytes(wire, 2, GameTitle::MM6);
    ASSERT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], firstValues);
    EXPECT_EQ(actual[1], secondValues);
    EXPECT_EQ(wire.bytes, bytes);
    EXPECT_THROW((void) mm6::decodeNativeActorResistanceBytes(wire, 1, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeNativeActorResistanceBytes(wire, 2, GameTitle::MM7), std::exception);

    mm6::OutdoorDeltaWire stale = wire;
    stale.actorCount = 1;
    EXPECT_THROW((void) mm6::decodeNativeActorResistanceBytes(stale, 2, GameTitle::MM6), std::exception);
    stale = wire;
    stale.bytes.pop_back();
    EXPECT_THROW((void) mm6::decodeNativeActorResistanceBytes(stale, 2, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6NativeActorResistanceBytes, RetainsIndoorActorOverridesFromCountedDlvSection) {
    std::vector<std::uint8_t> bytes(4 + 2 * 548);
    bytes[0] = 2;
    const std::array<std::uint8_t, 6> firstValues = {1, 2, 3, 4, 5, 6};
    const std::array<std::uint8_t, 6> secondValues = {10, 20, 30, 40, 50, 60};
    for (size_t i = 0; i < 6; ++i) {
        bytes[4 + 80 + i] = firstValues[i];
        bytes[4 + 548 + 80 + i] = secondValues[i];
    }
    MM6IndoorSnapshot source;
    source.isDelta = true;
    source.sections.push_back({MM6IndoorSectionKind::Actors,
                               Blob::copy(bytes.data(), bytes.size()), 2});
    const auto actual = mm6::decodeNativeIndoorActorResistanceBytes(
        source, 2, GameTitle::MM6);
    ASSERT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], firstValues);
    EXPECT_EQ(actual[1], secondValues);
    EXPECT_THROW((void) mm6::decodeNativeIndoorActorResistanceBytes(
        source, 1, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeNativeIndoorActorResistanceBytes(
        source, 2, GameTitle::MM7), std::exception);
    source.isDelta = false;
    EXPECT_THROW((void) mm6::decodeNativeIndoorActorResistanceBytes(
        source, 2, GameTitle::MM6), std::exception);
    source.isDelta = true;
    source.sections.front().count = 1;
    EXPECT_THROW((void) mm6::decodeNativeIndoorActorResistanceBytes(
        source, 2, GameTitle::MM6), std::exception);
    source.sections.front().count = 2;
    bytes[0] = 1;
    source.sections.front().bytes = Blob::copy(bytes.data(), bytes.size());
    EXPECT_THROW((void) mm6::decodeNativeIndoorActorResistanceBytes(
        source, 2, GameTitle::MM6), std::exception);
}
