#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V3FullSessionPreflight.h"
#include "Engine/Tables/ItemTable.h"

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
    std::vector<std::uint8_t> bytes(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
    for (std::size_t index = 0; index < 4; ++index)
        bytes[160 + index * 4 + 2] = static_cast<std::uint8_t>(1 + index * 36);
    return bytes;
}

std::vector<std::uint8_t> outdoorDelta(bool active, MonsterId monster = MONSTER_GOBLIN_A) {
    std::vector<std::uint8_t> bytes(8 + 2 * 88 * 11, 0);
    bytes[4] = active ? 1 : 0; // Saved day one suppresses initial respawn.
    append32(&bytes, active ? 1 : 0);
    if (active) {
        std::array<std::uint8_t, 548> actor{};
        actor[52] = static_cast<std::uint8_t>(monster);
        bytes.insert(bytes.end(), actor.begin(), actor.end());
    }
    append32(&bytes, 0);
    append32(&bytes, 0);
    bytes.resize(bytes.size() + 200 + 56, 0);
    return bytes;
}

Actor actor(MonsterId monster = MONSTER_GOBLIN_A) {
    Actor result(0);
    result.monsterInfo.id = monster;
    result.monsterId = MONSTER_INVALID;
    result.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    result.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    result.hp = 37; // ACTR may differ from its native DDM actor record.
    return result;
}

mm6::V3SaveFields exampleFields() {
    mm6::V3SaveFields fields;
    fields.raw.core.party.playingTicks = 42;
    fields.raw.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : fields.raw.core.characters)
        character.name = "Hero";
    fields.raw.inventory.ready = true;
    fields.raw.metadata.header.displayName = "Full session";
    fields.raw.metadata.header.mapFileName = "outside0.odm";
    fields.raw.metadata.header.playingTicks = 42;
    fields.raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        fields.raw.maps.deltas.push_back({"inside" + std::to_string(index) + ".dlv", indoorDelta()});
    for (int index = 0; index < 15; ++index)
        fields.raw.maps.deltas.push_back({"outside" + std::to_string(index) + ".ddm",
                                         outdoorDelta(index == 0)});
    fields.raw.chest = {17, {}, "outside0.ddm", fields.raw.maps.deltas[52].bytes};
    fields.raw.npcs.activeCount = 399;
    fields.raw.recovery.playingTicks = 42;
    fields.raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const std::array<Actor, 1> actors{actor()};
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

mm6::NpcStaticBaseline freshNpcs() {
    const auto table = [](std::string_view text) { return Blob::copy(text.data(), text.size()); };
    return mm6::loadNpcStaticBaseline(
        table("Header\r\nHeader\r\n1\tStatic Ada\t123\t0\t0\t0\t1\t2\t1\t7\t11\t12\t13\r\n"),
        table("0\tNews\tunused\tTown news\r\n"),
        table("Header\r\nAdam\tBeth\r\n"),
        table("1\tProfession\t2\t100\tunused\tAction\tBenefit\tJoin\r\n"),
        GameTitle::MM6);
}

struct ItemTableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    ItemTableScope() {
        pItemTable = table.get();
        const auto id = static_cast<ItemId>(10);
        table->items[id].iconName = "held-item";
        table->items[id].type = static_cast<ItemType>(3);
        table->items[id].rarity = RARITY_COMMON;
        table->itemSizes[id] = Sizei(1, 1);
    }

    ~ItemTableScope() { pItemTable = previous; }
};

mm6::V3FullSessionPreflight stage(const mm6::V3SaveFields &fields,
                                   const mm6::NpcStaticBaseline &npcs,
                                   const ItemTable &items,
                                   RandomEngineType selected = RANDOM_ENGINE_SEQUENTIAL) {
    const auto names = expectedNames(fields);
    return mm6::stageV3FullSessionPreflight(
        mm6::encodeV3Save(fields, GameTitle::MM6), npcs, names,
        baseReader, validItem, items, 5, 145, 10, selected, GameTitle::MM6);
}

} // namespace

UNIT_TEST(MM6V3FullSessionPreflight, StagesRepresentedOwnersAndPreEventState) {
    ItemTableScope items;
    const auto npcs = freshNpcs();
    auto fields = exampleFields();
    fields.raw.recovery.armageddonTimerTicks = 41;
    fields.raw.recovery.armageddonDamage = 12;
    fields.raw.recovery.armageddonForceCount = 3;
    const auto staged = stage(fields, npcs, *items.table);
    ASSERT_TRUE(staged.represented.outdoorPlan);
    EXPECT_FALSE(staged.represented.indoorPlan);
    EXPECT_EQ(staged.represented.outdoorPlan->mapName, "outside0.odm");
    EXPECT_EQ(staged.represented.world.world.mapDeltas.size(), 67);
    EXPECT_EQ(staged.actorMap.activeActors[0].hp, 37);
    EXPECT_EQ(staged.preEvent.generatedCount, 0);
    EXPECT_EQ(staged.preEvent.gameplayRng, fields.gameplayRng);
    EXPECT_FALSE(staged.cursorImage.occupied);
    EXPECT_EQ(staged.represented.party.party.armageddon_timer.ticks(), 41);
    EXPECT_EQ(staged.armageddon.timerTicks, 41);
    EXPECT_EQ(staged.armageddon.damage, 12);
    EXPECT_EQ(staged.armageddon.forceCount, 3);
}

UNIT_TEST(MM6V3FullSessionPreflight, RefusesMissingPreEventAndCursorOwners) {
    ItemTableScope items;
    const auto npcs = freshNpcs();
    auto fields = exampleFields();
    EXPECT_THROW((void) stage(fields, npcs, *items.table, RANDOM_ENGINE_MERSENNE_TWISTER),
                 std::exception);

    fields.raw.heldItem.present = true;
    fields.raw.heldItem.record.setWord(0, 10);
    EXPECT_THROW((void) stage(fields, npcs, *items.table), std::exception);

    fields = exampleFields();
    constexpr MonsterId streetPeasant = static_cast<MonsterId>(122);
    fields.raw.maps.deltas[52].bytes = outdoorDelta(true, streetPeasant);
    fields.raw.chest.rawActiveDelta = fields.raw.maps.deltas[52].bytes;
    std::array<Actor, 1> actors{actor(streetPeasant)};
    actors[0].npcId = 5000; // A street NPC link that the empty GNPC section has no row for.
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    EXPECT_THROW((void) stage(fields, npcs, *items.table), std::exception);
}

UNIT_TEST(MM6V3FullSessionPreflight, RefusesOldManifestAndForeignTitle) {
    ItemTableScope items;
    const auto npcs = freshNpcs();
    const auto fields = exampleFields();
    const auto names = expectedNames(fields);
    EXPECT_THROW((void) mm6::stageV3FullSessionPreflight(
        mm6::encodeRawSave(fields.raw, GameTitle::MM6), npcs, names,
        baseReader, validItem, *items.table, 5, 145, 10,
        RANDOM_ENGINE_SEQUENTIAL, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageV3FullSessionPreflight(
        mm6::encodeV3Save(fields, GameTitle::MM6), npcs, names,
        baseReader, validItem, *items.table, 5, 145, 10,
        RANDOM_ENGINE_SEQUENTIAL, GameTitle::MM7), std::exception);
}
