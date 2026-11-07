#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Snapshots/EntitySnapshots.h"

namespace {

void put32(std::vector<std::uint8_t> *bytes, std::size_t offset, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        (*bytes)[offset + shift / 8] = value >> shift;
}

struct Fixture {
    SpriteObject sprite;
    std::vector<std::uint8_t> native;

    Fixture() {
        sprite.vPosition = Vec3f(1.25f, -2.5f, 3.75f);
        sprite.vVelocity = Vec3f(0.25f, 0.5f, -0.75f);
        sprite.timeSinceCreated = Duration::fromTicks(70000);
        sprite.tempLifetime = Duration::fromTicks(40000);
        sprite._lastParticleTime = Duration::fromTicks(123456);
        sprite._ticksPerParticle = Duration::fromTicks(3);
        constexpr std::size_t header = 8 + 2 * 88 * 11;
        native.resize(header + 4 + 4 + 100 + 4 + 200 + 56);
        put32(&native, header + 4, 1); // One native ground slot.
        SpriteObject_MM7 projected{};
        snapshot(sprite, &projected);
        auto *record = native.data() + header + 8;
        std::memcpy(record, &projected, 36);
        std::memcpy(record + 36, &projected.containing_item, 28);
        std::memcpy(record + 64,
                    reinterpret_cast<const std::uint8_t *>(&projected) + 72, 36);
    }

    mm6::SpriteRuntimeSaveFields capture() const {
        return mm6::captureSpriteRuntimeFields(
            "o0.odm", native, std::span(&sprite, 1), GameTitle::MM6);
    }
};

mm6::V4SaveFields v4WithNative(const std::vector<std::uint8_t> &native) {
    mm6::V4SaveFields fields;
    auto &v3 = fields.v3;
    v3.raw.core.party.playingTicks = 42;
    v3.raw.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : v3.raw.core.characters)
        character.name = "Hero";
    v3.raw.inventory.ready = true;
    v3.raw.metadata.header.displayName = "V5 candidate";
    v3.raw.metadata.header.mapFileName = "o0.odm";
    v3.raw.metadata.header.playingTicks = 42;
    v3.raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        v3.raw.maps.deltas.push_back({"d" + std::to_string(index) + ".dlv", {1}});
    for (int index = 0; index < 15; ++index)
        v3.raw.maps.deltas.push_back({"o" + std::to_string(index) + ".ddm",
                                      index == 0 ? native : std::vector<std::uint8_t>{2}});
    v3.raw.chest = {0x12345678, {}, "o0.ddm", native};
    v3.raw.npcs.activeCount = 399;
    v3.raw.recovery.playingTicks = 42;
    v3.raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const std::array<Actor, 0> actors{};
    v3.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    v3.generatedNpcs.mapName = "o0.odm";
    v3.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    v3.gameplayRng.sequentialState = 12345;
    mm6::EventRuntimeMapEntry event;
    event.mapName = "o0.odm";
    event.respawnCount = 1;
    event.lastRespawnDay = 5;
    fields.eventRuntime.maps.push_back(event);
    return fields;
}

} // namespace

UNIT_TEST(MM6V5SpriteRuntimeFields, ExactFractionalFullListRoundTrip) {
    Fixture fixture;
    const auto fields = fixture.capture();
    const auto section = mm6::encodeSpriteRuntimeSection(fields, GameTitle::MM6);
    const auto decoded = mm6::decodeSpriteRuntimeSection({{section}, 5}, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeSpriteRuntimeSection(decoded, GameTitle::MM6), section);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(decoded.records[0].runtime.vPosition.x),
              std::bit_cast<std::uint32_t>(fixture.sprite.vPosition.x));
    EXPECT_EQ(decoded.records[0].runtime.timeSinceCreated.ticks(), 70000);
    EXPECT_EQ(decoded.records[0].runtime._lastParticleTime.ticks(), 123456);
    EXPECT_NO_THROW(mm6::requireSpriteRuntimeNativeAgreement(
        decoded, "o0.odm", fixture.native, GameTitle::MM6));
    fixture.native[8 + 2 * 88 * 11 + 8] ^= 1;
    EXPECT_THROW(mm6::requireSpriteRuntimeNativeAgreement(
        decoded, "o0.odm", fixture.native, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V5SpriteRuntimeFields, RejectsNonfiniteGeometryAndItemWithoutProvenance) {
    Fixture fixture;
    auto fields = fixture.capture();
    fields.records[0].runtime.vPosition.x = std::bit_cast<float>(0x7f800000u);
    EXPECT_THROW((void) mm6::encodeSpriteRuntimeSection(fields, GameTitle::MM6), std::exception);
    fields = fixture.capture();
    fields.records[0].runtime.containing_item.itemId = static_cast<ItemId>(200);
    EXPECT_THROW((void) mm6::encodeSpriteRuntimeSection(fields, GameTitle::MM6), std::exception);
    fields = fixture.capture();
    fields.records.push_back(fields.records.front());
    EXPECT_THROW((void) mm6::encodeV5Save(
        {v4WithNative(fixture.native), fields}, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V5SpriteRuntimeFields, ExactTwentyFiveSectionManifestAndOlderIsolation) {
    Fixture fixture;
    mm6::V5SaveFields fields{v4WithNative(fixture.native), fixture.capture(), {}};
    fields.actors = mm6::captureActorRuntimeFields(
        "o0.odm", fields.v4.v3.activeActorPayload,
        std::span<const Actor>{}, GameTitle::MM6);
    const auto bytes = mm6::encodeV5Save(fields, GameTitle::MM6);
    const auto container = mm6::decodeSaveContainer(bytes);
    EXPECT_EQ(container.version, 5);
    EXPECT_EQ(container.sections.size(), 25);
    const auto decoded = mm6::decodeV5Save(bytes, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeV5Save(decoded, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeV4Save(bytes, GameTitle::MM6), std::exception);
    fields.v4.v3.raw.chest.rawActiveDelta[8 + 2 * 88 * 11 + 8] ^= 1;
    EXPECT_THROW((void) mm6::encodeV5Save(fields, GameTitle::MM6), std::exception);
}
