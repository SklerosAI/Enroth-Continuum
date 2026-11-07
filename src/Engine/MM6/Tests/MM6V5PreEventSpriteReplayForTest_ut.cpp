#include <bit>
#include <cstdint>
#include <cstring>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V5PreEventSpriteReplayForTest.h"
#include "Engine/Snapshots/EntitySnapshots.h"

namespace {

void put32(std::vector<std::uint8_t> *bytes, std::size_t offset, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        (*bytes)[offset + shift / 8] = value >> shift;
}

struct Fixture {
    SpriteObject saved;
    std::vector<std::uint8_t> native;

    Fixture() {
        saved.vPosition = Vec3f(1.25f, -2.5f, 3.75f);
        saved.vVelocity = Vec3f(0.25f, 0.5f, -0.75f);
        saved._lastParticleTime = Duration::fromTicks(1234);
        constexpr std::size_t header = 8 + 2 * 88 * 11;
        native.resize(header + 4 + 4 + 100 + 4 + 200 + 56);
        put32(&native, header + 4, 1);
        SpriteObject_MM7 projected{};
        snapshot(saved, &projected);
        std::uint8_t *record = native.data() + header + 8;
        std::memcpy(record, &projected, 36);
        std::memcpy(record + 36, &projected.containing_item, 28);
        std::memcpy(record + 64,
                    reinterpret_cast<const std::uint8_t *>(&projected) + 72, 36);
    }

    mm6::SpriteRuntimeSaveFields fields() const {
        return mm6::captureSpriteRuntimeFields(
            "oute3.odm", native, std::span(&saved, 1), GameTitle::MM6);
    }
};

} // namespace

UNIT_TEST(MM6V5PreEventSpriteReplayForTest, RequiresActorThenRestoresExactFractionalSprite) {
    Fixture fixture;
    std::vector<SpriteObject> fresh(1);
    mm6::testing::ScopedV5PreEventSpriteReplayForTest scope(
        GameTitle::MM6, fixture.fields());
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, fresh, &fresh, false,
        "oute3.odm", fixture.native, GameTitle::MM6), std::exception);
    EXPECT_TRUE(mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, fresh, &fresh, true,
        "oute3.odm", fixture.native, GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_EQ(std::bit_cast<std::uint32_t>(fresh[0].vPosition.x),
              std::bit_cast<std::uint32_t>(fixture.saved.vPosition.x));
    EXPECT_EQ(fresh[0]._lastParticleTime, fixture.saved._lastParticleTime);
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, fresh, &fresh, true,
        "oute3.odm", fixture.native, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V5PreEventSpriteReplayForTest, RefusesChangedNativeAnchor) {
    Fixture fixture;
    std::vector<SpriteObject> fresh(1);
    mm6::testing::ScopedV5PreEventSpriteReplayForTest scope(
        GameTitle::MM6, fixture.fields());
    fixture.native[8 + 2 * 88 * 11 + 8] ^= 1;
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, fresh, &fresh, true,
        "oute3.odm", fixture.native, GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
}

UNIT_TEST(MM6V5PreEventSpriteReplayForTest, PublishesExactEmptyNativeSlotList) {
    Fixture fixture;
    const auto oneSlotNative = fixture.native;
    constexpr std::size_t groundStart = 8 + 2 * 88 * 11 + 8;
    fixture.native.erase(fixture.native.begin() + groundStart,
                         fixture.native.begin() + groundStart + 100);
    put32(&fixture.native, groundStart - 4, 0);
    std::vector<SpriteObject> empty;
    const auto fields = mm6::captureSpriteRuntimeFields(
        "oute2.odm", fixture.native, empty, GameTitle::MM6);
    ASSERT_TRUE(fields.records.empty());
    mm6::testing::ScopedV5PreEventSpriteReplayForTest scope(
        GameTitle::MM6, fields);
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, empty, &empty, true,
        "oute3.odm", fixture.native, GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, empty, &empty, true,
        "oute2.odm", oneSlotNative, GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
    EXPECT_TRUE(mm6::testing::runArmedV5PreEventSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, empty, &empty, true,
        "oute2.odm", fixture.native, GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_TRUE(empty.empty());
}
