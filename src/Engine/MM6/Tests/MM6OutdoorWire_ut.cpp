#include <cstddef>
#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorWire.h"

UNIT_TEST(MM6OutdoorWire, BaseBoundsAndRoundTrip) {
    constexpr size_t headerBytes = 160 + 16 + 3 * 128 * 128;
    constexpr size_t decorationMapBytes = 128 * 128 * 4;
    std::vector<std::uint8_t> base(headerBytes + 4 + 4 + 4 + decorationMapBytes + 4, 0);

    mm6::OutdoorBaseWire parsed = mm6::decodeOutdoorBase(base);
    EXPECT_EQ(parsed.modelCount, 0);
    EXPECT_EQ(parsed.decorationCount, 0);
    EXPECT_EQ(mm6::encodeOutdoorBase(parsed), base);

    std::vector<std::uint8_t> badCount = base;
    badCount[headerBytes] = 255;
    EXPECT_THROW((void) mm6::decodeOutdoorBase(badCount), std::exception);

    base.pop_back();
    EXPECT_THROW((void) mm6::decodeOutdoorBase(base), std::exception);
}

UNIT_TEST(MM6OutdoorWire, DeltaOpaqueMutationAndBounds) {
    constexpr size_t visibilityBytes = 2 * 88 * 11;
    std::vector<std::uint8_t> delta(8 + visibilityBytes + 3 * 4 + 200 + 56, 0);
    mm6::OutdoorDeltaWire parsed = mm6::decodeOutdoorDelta(delta);
    EXPECT_EQ(parsed.actorCount, 0);
    EXPECT_EQ(parsed.groundItemCount, 0);
    EXPECT_EQ(parsed.chestCount, 0);
    EXPECT_EQ(mm6::encodeOutdoorDelta(parsed), delta);

    parsed.part(mm6::OutdoorDeltaPart::ResetHeader)[1] = 37;
    parsed.part(mm6::OutdoorDeltaPart::Visibility)[17] = 128;
    parsed.part(mm6::OutdoorDeltaPart::PersistentBytes)[199] = 91;
    parsed.part(mm6::OutdoorDeltaPart::Tail)[55] = 42;
    std::vector<std::uint8_t> changed = mm6::encodeOutdoorDelta(parsed);
    EXPECT_EQ(mm6::decodeOutdoorDelta(changed).part(mm6::OutdoorDeltaPart::Tail)[55], 42);

    std::vector<std::uint8_t> badCount = delta;
    badCount[8 + visibilityBytes] = 255;
    EXPECT_THROW((void) mm6::decodeOutdoorDelta(badCount), std::exception);

    delta.pop_back();
    EXPECT_THROW((void) mm6::decodeOutdoorDelta(delta), std::exception);
}

UNIT_TEST(MM6OutdoorWire, CheckedModelFacetLookup) {
    constexpr size_t headerBytes = 160 + 16 + 3 * 128 * 128;
    constexpr size_t decorationMapBytes = 128 * 128 * 4;
    constexpr size_t modelExtraOffset = headerBytes + 4 + 188;
    std::vector<std::uint8_t> base(headerBytes + 4 + 188 + 2 * (308 + 2 + 10) +
                                   4 + 4 + decorationMapBytes + 4, 0);
    base[headerBytes] = 1; // One model.
    base[headerBytes + 4 + 76] = 2; // Two faces in the model header.
    base[modelExtraOffset + 308] = 0x5A; // A byte in face 1.
    base[modelExtraOffset + 2 * 308 + 2 * 2 + 10] = 'X'; // Texture name for face 1.

    mm6::OutdoorBaseWire parsed = mm6::decodeOutdoorBase(base);
    EXPECT_EQ(parsed.faceBytes(0, 1)[0], 0x5A);
    EXPECT_EQ(parsed.textureNameBytes(0, 1)[0], 'X');
    parsed.faceBytes(0, 1)[0] = 0xA5;
    parsed.textureNameBytes(0, 1)[0] = 'Y';
    EXPECT_EQ(mm6::decodeOutdoorBase(mm6::encodeOutdoorBase(parsed)).faceBytes(0, 1)[0], 0xA5);
    EXPECT_THROW((void) parsed.faceBytes(1, 0), std::exception);
    EXPECT_THROW((void) parsed.faceBytes(0, 2), std::exception);
    EXPECT_THROW((void) parsed.textureNameBytes(0, 2), std::exception);
}
