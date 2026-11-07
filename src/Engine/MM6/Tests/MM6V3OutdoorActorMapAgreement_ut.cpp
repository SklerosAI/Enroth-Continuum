#include <array>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Core/Time/Time.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V3OutdoorActorMapAgreement.h"

namespace {

void append32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(static_cast<std::uint8_t>(value >> shift));
}

std::vector<std::uint8_t> indoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200;
    return bytes;
}

std::vector<std::uint8_t> indoorDelta() {
    return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

std::vector<std::uint8_t> outdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

std::vector<std::uint8_t> outdoorDelta(std::uint32_t day, bool nativeActor) {
    std::vector<std::uint8_t> bytes(8 + 2 * 88 * 11, 0);
    bytes[4] = static_cast<std::uint8_t>(day);
    append32(&bytes, nativeActor ? 1 : 0);
    if (nativeActor) {
        std::array<std::uint8_t, 548> actor{};
        actor[52] = 7; // MonsterInfo ID, the stable MM6 native anchor.
        bytes.insert(bytes.end(), actor.begin(), actor.end());
    }
    append32(&bytes, 0); // Ground items.
    append32(&bytes, 0); // Chests.
    bytes.resize(bytes.size() + 200 + 56, 0);
    return bytes;
}

Actor actor(int slot) {
    Actor result(slot);
    result.monsterInfo.id = static_cast<MonsterId>(7);
    result.monsterId = MONSTER_INVALID;
    result.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    result.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    return result;
}

mm6::V3SaveFields exampleFields() {
    mm6::V3SaveFields fields;
    fields.raw.core.party.playingTicks = 42;
    fields.raw.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : fields.raw.core.characters)
        character.name = "Hero";
    fields.raw.inventory.ready = true;
    fields.raw.metadata.header.displayName = "V3 outdoor";
    fields.raw.metadata.header.mapFileName = "outside0.odm";
    fields.raw.metadata.header.playingTicks = 42;
    fields.raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        fields.raw.maps.deltas.push_back({"inside" + std::to_string(index) + ".dlv", indoorDelta()});
    for (int index = 0; index < 15; ++index)
        fields.raw.maps.deltas.push_back({"outside" + std::to_string(index) + ".ddm",
                                         outdoorDelta(index == 0 ? 1 : 0, index == 0)});
    fields.raw.chest = {17, {}, "outside0.ddm", fields.raw.maps.deltas[52].bytes};
    fields.raw.npcs.activeCount = 399;
    fields.raw.recovery.playingTicks = 42;
    fields.raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const std::array<Actor, 2> actors{actor(0), actor(1)};
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    fields.generatedNpcs.mapName = "outside0.odm";
    fields.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    fields.gameplayRng.sequentialState = 29;
    return fields;
}

std::vector<std::string> expectedNames(const mm6::V3SaveFields &fields) {
    std::vector<std::string> names;
    for (const mm6::SavedMapDelta &delta : fields.raw.maps.deltas)
        names.push_back(delta.name);
    return names;
}

Blob baseReader(std::string_view name) {
    const auto bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
    return Blob::copy(bytes.data(), bytes.size());
}

bool validItem(std::int32_t id) { return id >= 1 && id <= 580; }

mm6::V3OutdoorActorMapAgreement stage(const mm6::V3SaveFields &fields, size_t cap = 3) {
    const auto names = expectedNames(fields);
    return mm6::stageV3OutdoorActorMapAgreement(
        mm6::encodeV3Save(fields, GameTitle::MM6), names,
        baseReader, validItem, cap, GameTitle::MM6);
}

} // namespace

UNIT_TEST(MM6V3OutdoorActorMapAgreement, BindsNativeAndSpawnedActorSlots) {
    auto fields = exampleFields();
    auto actors = mm6::decodeActiveActorSection(fields.activeActorPayload, GameTitle::MM6);
    actors[0].hp = 37; // ACTR owns changed gameplay state; DDM keeps its native anchor.
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    const auto staged = stage(fields);
    EXPECT_EQ(staged.activeMap.mapName, "outside0.odm");
    EXPECT_EQ(staged.activeMap.delta.lastRespawnDay, 1);
    ASSERT_EQ(staged.activeMap.delta.actors.size(), 1);
    ASSERT_EQ(staged.activeActors.size(), 2);
    EXPECT_EQ(staged.activeActors[0].id, 0);
    EXPECT_EQ(staged.activeActors[0].hp, 37);
    EXPECT_EQ(staged.activeActors[1].id, 1);
    EXPECT_EQ(staged.saved.activeActorPayload, fields.activeActorPayload);
}

UNIT_TEST(MM6V3OutdoorActorMapAgreement, RejectsRespawnAndActorIdentityDivergence) {
    auto fields = exampleFields();
    fields.raw.maps.deltas[52].bytes = outdoorDelta(0, true);
    fields.raw.chest.rawActiveDelta = fields.raw.maps.deltas[52].bytes;
    EXPECT_THROW((void) stage(fields), std::exception);

    fields = exampleFields();
    fields.raw.chest.rawActiveDelta = outdoorDelta(2, true);
    EXPECT_THROW((void) stage(fields), std::exception);

    fields = exampleFields();
    fields.raw.chest.rawActiveDelta = outdoorDelta(1, true);
    fields.raw.chest.rawActiveDelta[8 + 2 * 88 * 11 + 4 + 52] = 8;
    EXPECT_THROW((void) stage(fields), std::exception);

    fields = exampleFields();
    const std::array<Actor, 2> changed{actor(0), actor(1)};
    auto actors = changed;
    actors[0].monsterInfo.id = static_cast<MonsterId>(8);
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    EXPECT_THROW((void) stage(fields), std::exception);

    fields = exampleFields();
    fields.raw.core.party.playingTicks = Time::fromDays(0x1BAF800 + 1).ticks();
    fields.raw.metadata.header.playingTicks = fields.raw.core.party.playingTicks;
    fields.raw.recovery.playingTicks = fields.raw.core.party.playingTicks;
    EXPECT_THROW((void) stage(fields), std::exception);
}

UNIT_TEST(MM6V3OutdoorActorMapAgreement, RejectsCapMissingNativeAndMapMismatch) {
    auto fields = exampleFields();
    EXPECT_THROW((void) stage(fields, 1), std::exception);
    EXPECT_THROW((void) stage(fields, 0), std::exception);

    const std::array<Actor, 0> none{};
    fields.activeActorPayload = mm6::encodeActiveActorSection(none, GameTitle::MM6);
    EXPECT_THROW((void) stage(fields), std::exception);

    fields = exampleFields();
    fields.raw.chest.activeDeltaName = "outside1.ddm";
    EXPECT_THROW((void) stage(fields), std::exception);
    fields = exampleFields();
    fields.generatedNpcs.mapName = "outside1.odm";
    EXPECT_THROW((void) stage(fields), std::exception);
}
