#include <array>
#include <bit>
#include <cstring>
#include <deque>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Engine.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Engine/MM6/MM6V6IndoorPreEventOwnerGate.h"
#include "Engine/MM6/MM6V6PreEventIndoorDecorationReplayForTest.h"
#include "Engine/MM6/MM6V6PreEventIndoorSpriteReplayForTest.h"
#include "Engine/MM6/MM6V6PreEventIndoorActorReplayForTest.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Library/Random/SequentialRandomEngine.h"
#include "Utility/Memory/Blob.h"

namespace {

mm6::V6IndoorSaveFields candidate() {
    mm6::V6IndoorSaveFields result;
    auto &raw = result.raw;
    raw.core.party.playingTicks = 42;
    raw.core.party.activeCharacter = 2;
    for (auto &character : raw.core.characters)
        character.name = "Hero";
    raw.inventory.ready = true;
    raw.metadata.header.displayName = "Indoor candidate";
    raw.metadata.header.mapFileName = "d0.blv";
    raw.metadata.header.playingTicks = 42;
    raw.metadata.ui.activeCharacter = 2;
    for (int index = 0; index < 52; ++index)
        raw.maps.deltas.push_back({"d" + std::to_string(index) + ".dlv", {1}});
    for (int index = 0; index < 15; ++index)
        raw.maps.deltas.push_back({"o" + std::to_string(index) + ".ddm", {2}});
    raw.chest = {0x12345678, {}, "d0.dlv", {1}};
    raw.npcs.activeCount = 399;
    raw.recovery.playingTicks = 42;
    raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);

    result.generatedNpcs.mapName = "d0.blv";
    SequentialRandomEngine rng;
    rng.seed(22);
    result.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    result.gameplayRng.sequentialState = rng.state();
    mm6::EventRuntimeMapEntry event;
    event.mapName = "d0.blv";
    event.lastRespawnDay = 5;
    result.eventRuntime.maps.push_back(event);
    const std::vector<mm6::IndoorFaceState> base{{1, "stone"}};
    const std::vector<mm6::IndoorFaceState> changed{{2, "stone"}};
    result.faceOverrides.maps.push_back(*mm6::captureIndoorFaceOverrides(
        "d0.blv", base, changed, GameTitle::MM6));
    result.actors = {"d0.blv", 0, {}};
    result.sprites = {"d0.blv", 0, {}};
    result.decorations = {"d0.blv", 3, std::vector<std::uint8_t>(180, 0x33)};
    result.runtimeDecorations = {"d0.blv", {{1, 0, 0}, {1, 0, 0}, {1, 0, 0}}};
    result.runtimeSprites.mapName = "d0.blv";
    result.runtimeActors.mapName = "d0.blv";
    return result;
}

mm6::SaveSection &section(mm6::SaveContainer *container,
                          const std::array<char, 4> &tag) {
    for (auto &item : container->sections)
        if (item.tag == tag)
            return item;
    throw std::runtime_error("MM6 v6 fixture section is absent");
}

std::vector<std::uint8_t> indoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200;
    return bytes;
}

std::vector<std::uint8_t> indoorDelta() {
    std::vector<std::uint8_t> bytes(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
    bytes[4] = 5; // Native last-respawn day agrees with EVRT.
    return bytes;
}

std::vector<std::uint8_t> outdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

std::vector<std::uint8_t> outdoorDelta() {
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

mm6::NpcStaticBaseline freshNpcBaseline() {
    const auto table = [](std::string_view value) {
        return Blob::copy(value.data(), value.size());
    };
    return mm6::loadNpcStaticBaseline(
        table("Header\r\nHeader\r\n1\tStatic Ada\t123\t0\t0\t0\t1\t2\t1\t7\t11\t12\t13\r\n"),
        table("0\tNews\tunused\tTown news\r\n"),
        table("Header\r\nAdam\tBeth\r\n"),
        table("1\tProfession\t2\t100\tunused\tAction\tBenefit\tJoin\r\n"),
        GameTitle::MM6);
}

mm6::V6IndoorSaveFields stagedCandidate() {
    auto fields = candidate();
    fields.faceOverrides.maps.clear();
    for (auto &delta : fields.raw.maps.deltas)
        delta.bytes = delta.name.ends_with(".dlv") ? indoorDelta() : outdoorDelta();
    fields.raw.chest.rawActiveDelta = indoorDelta();
    fields.actors.count = 0;
    fields.actors.records.clear();
    fields.sprites.count = 0;
    fields.sprites.records.clear();
    fields.decorations.count = 0;
    fields.decorations.records.clear();
    fields.runtimeDecorations.records.clear();
    return fields;
}

struct ItemTableScope {
    ItemTable table;
    ItemTable *previous = pItemTable;

    ItemTableScope() { pItemTable = &table; }
    ~ItemTableScope() { pItemTable = previous; }
};

} // namespace

UNIT_TEST(MM6V6IndoorSaveFields, StrictTwentyNineSectionRoundTrip) {
    const auto source = candidate();
    const auto bytes = mm6::encodeV6IndoorSave(source, GameTitle::MM6);
    const auto container = mm6::decodeSaveContainer(bytes);
    EXPECT_EQ(container.version, 6);
    EXPECT_EQ(container.sections.size(), 29);
    const auto decoded = mm6::decodeV6IndoorSave(bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.raw.metadata.header.mapFileName, "d0.blv");
    EXPECT_EQ(decoded.actors, source.actors);
    EXPECT_EQ(decoded.sprites, source.sprites);
    EXPECT_EQ(decoded.decorations, source.decorations);
    EXPECT_EQ(decoded.runtimeDecorations, source.runtimeDecorations);
    EXPECT_EQ(decoded.runtimeSprites.mapName, source.runtimeSprites.mapName);
    EXPECT_EQ(decoded.runtimeActors.mapName, source.runtimeActors.mapName);
    EXPECT_EQ(mm6::encodeV6IndoorSave(decoded, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeRawSave(bytes, GameTitle::MM6), std::exception);
    EXPECT_FALSE(mm6::StagedV6IndoorSaveFields{}.canEnterGameLoop());
}

UNIT_TEST(MM6V6IndoorSaveFields, RejectsMissingMismatchedAndMalformedOwners) {
    auto source = candidate();
    source.sprites.mapName = "oute3.odm";
    EXPECT_THROW((void) mm6::encodeV6IndoorSave(source, GameTitle::MM6), std::exception);
    source = candidate();
    source.actors.records.push_back(0);
    EXPECT_THROW((void) mm6::encodeV6IndoorSave(source, GameTitle::MM6), std::exception);
    source = candidate();
    EXPECT_THROW((void) mm6::encodeV6IndoorSave(source, GameTitle::MM7), std::exception);

    auto container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'A', 'C', 'R'}).payload[2] = 'x';
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'S', 'C', 'R'}).payload.pop_back();
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'F', 'O', 'V'}).required = true;
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'D', 'R', 'T'}).payload.pop_back();
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'S', 'R', 'T'}).payload.pop_back();
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    section(&container, {'I', 'A', 'R', 'T'}).payload.pop_back();
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
    container = mm6::decodeSaveContainer(
        mm6::encodeV6IndoorSave(candidate(), GameTitle::MM6));
    std::erase_if(container.sections, [](const mm6::SaveSection &part) {
        return part.tag == std::array<char, 4>{'I', 'D', 'R', 'T'};
    });
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(
        mm6::encodeSaveContainer(container), GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, DecorationRuntimeReplaysOnlyMutableSlots) {
    auto base = std::make_unique<IndoorLocation_MM7>();
    base->decorations.resize(1);
    base->decorationNames.resize(1);
    base->decorationNames[0][0] = 'd';
    base->decorationNames[0][1] = 'e';
    base->decorationNames[0][2] = 'c';
    base->decorationNames[0][3] = '0';
    base->decorationNames[0][4] = '1';
    DecorationTable table;
    table.decorations.resize(147);
    table.decorations[146].internalName = "dec01";
    mm6::EventRuntimeMapEntry event;
    event.mapName = "d0.blv";
    std::vector<LevelDecoration> live(1);
    reconstruct(base->decorations[0], &live[0]);
    live[0].uDecorationDescID = static_cast<DecorationId>(146);
    live[0].eventVarId = base->decorations[0].eventVarId;
    event.bindings = mm6::captureEventRuntimeDecorationBindings(
        live, GameTitle::MM6);
    live[0].uFlags |= LEVEL_DECORATION_INVISIBLE;
    const auto fields = mm6::captureIndoorDecorationRuntime(
        event.mapName, *base, event, table, live, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeIndoorDecorationRuntimeSection(
        fields, GameTitle::MM6));
    const auto decoded = mm6::decodeIndoorDecorationRuntimeSection(
        container, GameTitle::MM6);
    EXPECT_EQ(decoded, fields);
    auto replay = mm6::stageIndoorDecorationRuntime(
        decoded, *base, event, table, GameTitle::MM6);
    EXPECT_EQ(replay[0].uFlags, live[0].uFlags);
    replay[0].uFlags = {};
    mm6::applyIndoorDecorationRuntimeAfterMapReload(
        decoded, *base, event, table, &replay, GameTitle::MM6);
    EXPECT_EQ(replay[0].uFlags, live[0].uFlags);
    replay[0].vPosition.x += 1;
    EXPECT_THROW(mm6::applyIndoorDecorationRuntimeAfterMapReload(
        decoded, *base, event, table, &replay, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, DecorationRuntimeKeepsNullSourceDescriptorSlot) {
    auto base = std::make_unique<IndoorLocation_MM7>();
    base->decorations.resize(1);
    base->decorationNames.resize(1); // Empty native name resolves to sentinel slot zero.
    DecorationTable table;
    table.decorations.resize(1);
    mm6::EventRuntimeMapEntry event;
    event.mapName = "d0.blv";
    std::vector<LevelDecoration> live(1);
    reconstruct(base->decorations[0], &live[0]);
    live[0].eventVarId = base->decorations[0].eventVarId;
    const auto fields = mm6::captureIndoorDecorationRuntime(
        event.mapName, *base, event, table, live, GameTitle::MM6);
    ASSERT_EQ(fields.records.size(), 1u);
    EXPECT_EQ(fields.records[0].descriptorId, 0u);
    const auto replay = mm6::stageIndoorDecorationRuntime(
        fields, *base, event, table, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 1u);
    EXPECT_EQ(replay[0].uDecorationDescID, DECORATION_NULL);
}

UNIT_TEST(MM6V6IndoorSaveFields, IndoorDecorationPublicationRequiresExactPreEventContext) {
    std::vector<LevelDecoration> live(1);
    std::vector<LevelDecoration> saved = live;
    saved[0].uFlags |= LEVEL_DECORATION_INVISIBLE;
    mm6::testing::ScopedV6PreEventIndoorDecorationReplayForTest scope(
        GameTitle::MM6, "d0.blv", saved);
    EXPECT_THROW((void) mm6::testing::runArmedV6PreEventIndoorDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::AbortToMainMenu,
        live, &live, true, "d0.blv", GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
    EXPECT_TRUE(mm6::testing::runArmedV6PreEventIndoorDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue,
        live, &live, true, "d0.blv", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_EQ(live[0].uFlags, saved[0].uFlags);
    EXPECT_THROW((void) mm6::testing::runArmedV6PreEventIndoorDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue,
        live, &live, true, "d0.blv", GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, IndoorSpriteOwnerPreservesFractionalStateAndNativeItem) {
    SpriteObject live;
    live.vPosition = Vec3f(1.25f, -2.5f, 3.75f);
    live.vVelocity = Vec3f(0.25f, 0.5f, -0.75f);
    live.timeSinceCreated = Duration::fromTicks(70000);
    mm6::IndoorNativeSlotAnchor source{"d0.blv", 1,
        std::vector<std::uint8_t>(100)};
    SpriteObject_MM7 projected{};
    snapshot(live, &projected);
    std::memcpy(source.records.data(), &projected, 36);
    std::memcpy(source.records.data() + 36, &projected.containing_item, 28);
    std::memcpy(source.records.data() + 64,
                reinterpret_cast<const std::uint8_t *>(&projected) + 72, 36);
    const auto captured = mm6::captureIndoorSpriteRuntime(
        "d0.blv", source, std::span(&live, 1), GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeSpriteRuntimeSectionAs(
        captured, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6));
    const auto decoded = mm6::decodeSpriteRuntimeSectionAs(
        container, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6);
    auto detached = mm6::stageIndoorSpriteRuntime(
        decoded, "d0.blv", source, GameTitle::MM6);
    ASSERT_EQ(detached.size(), 1u);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(detached[0].vPosition.x),
              std::bit_cast<std::uint32_t>(live.vPosition.x));
    EXPECT_EQ(detached[0].timeSinceCreated.ticks(), 70000);
    std::array<SpriteObject, 2> withAppend{live, live};
    withAppend[1].initialPosition = Vec3f(1.25f, 2.5f, -3.75f);
    withAppend[1].containing_item.potionPower = 17;
    withAppend[1].containing_item.goldAmount = 23;
    const auto appended = mm6::captureIndoorSpriteRuntime(
        "d0.blv", source, withAppend, GameTitle::MM6);
    EXPECT_EQ(appended.nativePrefixCount, 1u);
    const auto appendedSection = mm6::encodeSpriteRuntimeSectionAs(
        appended, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6);
    mm6::SaveContainer appendedContainer;
    appendedContainer.sections.push_back(appendedSection);
    const auto appendedDecoded = mm6::decodeSpriteRuntimeSectionAs(
        appendedContainer, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6);
    const auto appendedReplay = mm6::stageIndoorSpriteRuntime(
        appendedDecoded, "d0.blv", source, GameTitle::MM6);
    ASSERT_EQ(appendedReplay.size(), 2u);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(appendedReplay[1].initialPosition.x),
              std::bit_cast<std::uint32_t>(withAppend[1].initialPosition.x));
    EXPECT_EQ(appendedReplay[1].containing_item.potionPower, 17);
    EXPECT_EQ(appendedReplay[1].containing_item.goldAmount, 23);
    EXPECT_EQ(mm6::encodeSpriteRuntimeSectionAs(
        appendedDecoded, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6),
        appendedSection);
    detached[0].vPosition.x = 0;
    mm6::applyIndoorSpriteRuntimeAfterMapReload(
        decoded, "d0.blv", source, &detached, GameTitle::MM6);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(detached[0].vPosition.x),
              std::bit_cast<std::uint32_t>(live.vPosition.x));
    source.records[0] ^= 1;
    EXPECT_THROW((void) mm6::stageIndoorSpriteRuntime(
        decoded, "d0.blv", source, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, IndoorSpriteOwnerTracksReusedNativeFreeSlot) {
    mm6::IndoorNativeSlotAnchor source{"d0.blv", 1,
        std::vector<std::uint8_t>(100)};
    SpriteObject spawned;
    spawned.spriteId = SPRITE_PROJECTILE_AIR_BOLT;
    spawned.uObjectDescID = 1;
    spawned.initialPosition = Vec3f(1.25f, 2.5f, 3.75f);
    const auto fields = mm6::captureIndoorSpriteRuntime(
        "d0.blv", source, std::span(&spawned, 1), GameTitle::MM6);
    EXPECT_EQ(fields.nativePrefixCount, 1u);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeSpriteRuntimeSectionAs(
        fields, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6));
    const auto decoded = mm6::decodeSpriteRuntimeSectionAs(
        container, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6);
    const auto replay = mm6::stageIndoorSpriteRuntime(
        decoded, "d0.blv", source, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 1u);
    EXPECT_EQ(replay[0].spriteId, spawned.spriteId);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(replay[0].initialPosition.x),
              std::bit_cast<std::uint32_t>(spawned.initialPosition.x));
    source.records[2] = 1; // An occupied source descriptor cannot be reused.
    EXPECT_THROW((void) mm6::captureIndoorSpriteRuntime(
        "d0.blv", source, std::span(&spawned, 1), GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, IndoorSpritePublicationRequiresPreEventContext) {
    mm6::IndoorNativeSlotAnchor source{"d0.blv", 0, {}};
    std::vector<SpriteObject> saved(1);
    saved[0].spriteId = SPRITE_PROJECTILE_AIR_BOLT;
    saved[0].uObjectDescID = 1;
    const auto fields = mm6::captureIndoorSpriteRuntime(
        "d0.blv", source, saved, GameTitle::MM6);
    mm6::testing::ScopedV6PreEventIndoorSpriteReplayForTest scope(
        GameTitle::MM6, fields, source);
    std::vector<SpriteObject> rebuilt;
    EXPECT_THROW((void) mm6::testing::runArmedV6PreEventIndoorSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::AbortToMainMenu,
        rebuilt, &rebuilt, true, "d0.blv", GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
    EXPECT_TRUE(mm6::testing::runArmedV6PreEventIndoorSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue,
        rebuilt, &rebuilt, true, "d0.blv", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    ASSERT_EQ(rebuilt.size(), 1u);
    EXPECT_EQ(rebuilt[0].spriteId, saved[0].spriteId);
    EXPECT_THROW((void) mm6::testing::runArmedV6PreEventIndoorSpriteReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue,
        rebuilt, &rebuilt, true, "d0.blv", GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, IndoorActorOwnerKeepsGeneratedFractionalTail) {
    mm6::IndoorNativeSlotAnchor source{"d0.blv", 0, {}};
    Actor actor(0);
    actor.monsterInfo.id = static_cast<MonsterId>(7);
    actor.monsterId = MONSTER_INVALID;
    actor.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    actor.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    actor.initialPosition = Vec3f(10, 20, 30);
    actor.pos = Vec3f(11.25f, 20.5f, 30.75f);
    actor.velocity = Vec3f(0.125f, -0.5f, 1.25f);
    actor.monsterInfo.name = "Indoor Display";
    actor.monsterInfo.internalName = "Indoor Internal";
    actor.npcId = -1;
    EXPECT_THROW((void) mm6::captureIndoorActorRuntime(
        "d0.blv", source, std::span(&actor, 1), GameTitle::MM6), std::exception);
    actor.npcId = 0;
    const auto fields = mm6::captureIndoorActorRuntime(
        "d0.blv", source, std::span(&actor, 1), GameTitle::MM6);
    EXPECT_EQ(fields.nativePrefixCount, 0u);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeActorRuntimeSectionAs(
        fields, mm6::ActorRuntimeSectionKind::IndoorV6, GameTitle::MM6));
    const auto decoded = mm6::decodeActorRuntimeSectionAs(
        container, mm6::ActorRuntimeSectionKind::IndoorV6, GameTitle::MM6);
    auto replay = mm6::stageIndoorActorRuntime(
        decoded, "d0.blv", source, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 1u);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(replay[0].pos.x),
              std::bit_cast<std::uint32_t>(actor.pos.x));
    EXPECT_EQ(replay[0].monsterInfo.name, actor.monsterInfo.name);
    EXPECT_EQ(replay[0].monsterInfo.internalName, actor.monsterInfo.internalName);
    replay.clear();
    mm6::applyIndoorActorRuntimeToDetachedList(
        decoded, "d0.blv", source, &replay, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 1u);
    EXPECT_EQ(mm6::encodeActorRuntimeSectionAs(
        decoded, mm6::ActorRuntimeSectionKind::IndoorV6, GameTitle::MM6),
        container.sections[0]);
    source.count = 1;
    EXPECT_THROW((void) mm6::stageIndoorActorRuntime(
        decoded, "d0.blv", source, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, IndoorActorPublicationRequiresPreEventContext) {
    mm6::IndoorNativeSlotAnchor source{"d0.blv", 0, {}};
    Actor actor(0);
    actor.monsterInfo.id = static_cast<MonsterId>(7);
    actor.monsterId = MONSTER_INVALID;
    actor.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    actor.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    const auto fields = mm6::captureIndoorActorRuntime(
        "d0.blv", source, std::span(&actor, 1), GameTitle::MM6);
    mm6::GeneratedNpcSaveFields generated;
    generated.mapName = "d0.blv";
    GameplayRandomState rng;
    rng.type = RANDOM_ENGINE_SEQUENTIAL;
    SequentialRandomEngine sequential;
    sequential.seed(22);
    rng.sequentialState = sequential.state();
    NPCStats liveNpcs;
    mm6::testing::ScopedV6PreEventIndoorActorReplayForTest scope(
        GameTitle::MM6, fields, source, generated, rng, true);
    std::deque<Actor> rebuilt;
    EXPECT_THROW((void) mm6::testing::runArmedV6PreEventIndoorActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::AbortToMainMenu,
        rebuilt, &rebuilt, &liveNpcs, RANDOM_ENGINE_SEQUENTIAL,
        true, "d0.blv", GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
    EXPECT_TRUE(mm6::testing::runArmedV6PreEventIndoorActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue,
        rebuilt, &rebuilt, &liveNpcs, RANDOM_ENGINE_SEQUENTIAL,
        true, "d0.blv", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_TRUE(mm6::testing::consumeV6IndoorGeneratedNpcBypassForTest(GameTitle::MM6));
    EXPECT_THROW(mm6::testing::requireV6IndoorReplayBeforeMapLoadForTest(GameTitle::MM6),
                 std::exception);
    mm6::testing::restoreV6IndoorRngBeforeMapLoadForTest(
        [](const GameplayRandomState &) {}, GameTitle::MM6);
    EXPECT_NO_THROW(mm6::testing::requireV6IndoorReplayBeforeMapLoadForTest(GameTitle::MM6));
    ASSERT_EQ(rebuilt.size(), 1u);
    EXPECT_EQ(rebuilt.front().monsterInfo.id, actor.monsterInfo.id);
}

UNIT_TEST(MM6V6IndoorSaveFields, NativeAnchorCaptureRequiresMatchingIndoorPlan) {
    mm6::StagedRawSessionFields represented;
    represented.indoorPlan.emplace();
    represented.indoorPlan->mapName = "d0.blv";
    represented.indoorPlan->savedDelta.wire.sections.push_back(
        {MM6IndoorSectionKind::Actors, Blob::fromString(std::string(4, '\0')), 0});
    represented.indoorPlan->savedDelta.wire.sections.push_back(
        {MM6IndoorSectionKind::GroundItems, Blob::fromString(std::string(4, '\0')), 0});
    MM6IndoorSnapshot base;
    base.sections.push_back(
        {MM6IndoorSectionKind::Sprites, Blob::fromString(std::string(4, '\0')), 0});
    represented.raw.activeMap = mm6::StagedIndoorMap{"d0.blv", std::move(base), {}};
    mm6::IndoorNativeSlotAnchor actors, sprites, decorations;
    mm6::captureV6IndoorNativeAnchors(represented, &actors, &sprites, &decorations);
    EXPECT_EQ(actors, (mm6::IndoorNativeSlotAnchor{"d0.blv", 0, {}}));
    EXPECT_EQ(sprites, (mm6::IndoorNativeSlotAnchor{"d0.blv", 0, {}}));
    EXPECT_EQ(decorations, (mm6::IndoorNativeSlotAnchor{"d0.blv", 0, {}}));

    represented.indoorPlan->mapName = "other.blv";
    EXPECT_THROW(mm6::captureV6IndoorNativeAnchors(
        represented, &actors, &sprites, &decorations), std::exception);
    represented.indoorPlan->mapName = "d0.blv";
    represented.indoorPlan->savedDelta.wire.sections[0].count = 1;
    EXPECT_THROW(mm6::captureV6IndoorNativeAnchors(
        represented, &actors, &sprites, &decorations), std::exception);
}

UNIT_TEST(MM6V6IndoorSaveFields, ValidatedRawV2IndoorStageRefusesGameplay) {
    const auto source = stagedCandidate();
    const auto bytes = mm6::encodeV6IndoorSave(source, GameTitle::MM6);
    std::vector<std::string> names;
    for (const auto &delta : source.raw.maps.deltas)
        names.push_back(delta.name);
    const auto readBase = [](std::string_view name) {
        const auto bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
        return Blob::copy(bytes.data(), bytes.size());
    };
    const auto baseline = [](std::string_view name) {
        if (name != "d0.blv")
            throw std::runtime_error("fixture requested another face base");
        return std::vector<mm6::IndoorFaceState>{};
    };
    ItemTableScope items;
    DecorationTable decorations;
    const auto npcs = freshNpcBaseline();
    auto staged = mm6::stageV6IndoorSave(
        bytes, npcs, names, readBase,
        [](std::int32_t id) { return id >= 1 && id <= 580; },
        items.table, decorations, 5, 145, baseline, GameTitle::MM6);
    ASSERT_TRUE(staged.represented.indoorPlan);
    EXPECT_FALSE(staged.represented.outdoorPlan);
    EXPECT_FALSE(staged.canEnterGameLoop());
    EXPECT_EQ(staged.saved.actors.count, 0u);
    EXPECT_NO_THROW(mm6::requireV6IndoorPreEventOwnerAgreement(
        staged, "d0.blv", {}, {}, {}, GameTitle::MM6));
    staged.saved.actors.count = 1;
    EXPECT_THROW(mm6::requireV6IndoorPreEventOwnerAgreement(
        staged, "d0.blv", {}, {}, {}, GameTitle::MM6), std::exception);
    staged.saved.actors.count = 0;

    auto changed = source;
    changed.actors.count = 1;
    changed.actors.records.assign(548, 0);
    EXPECT_THROW((void) mm6::stageV6IndoorSave(
        mm6::encodeV6IndoorSave(changed, GameTitle::MM6), npcs, names,
        readBase, [](std::int32_t id) { return id >= 1 && id <= 580; },
        items.table, decorations, 5, 145, baseline, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageV6IndoorSave(
        bytes, npcs, names, readBase,
        [](std::int32_t id) { return id >= 1 && id <= 580; },
        items.table, decorations, 5, 145, {}, GameTitle::MM6), std::exception);
}
