#include <array>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V4FullSessionPreflight.h"
#include "Engine/Objects/Actor.h"
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

std::vector<std::uint8_t> outdoorDelta(bool active) {
    std::vector<std::uint8_t> bytes(8 + 2 * 88 * 11, 0);
    bytes[4] = active ? 1 : 0;
    append32(&bytes, active ? 1 : 0);
    if (active) {
        std::array<std::uint8_t, 548> actor{};
        actor[52] = static_cast<std::uint8_t>(MONSTER_GOBLIN_A);
        bytes.insert(bytes.end(), actor.begin(), actor.end());
    }
    append32(&bytes, 0);
    append32(&bytes, 0);
    bytes.resize(bytes.size() + 200 + 56, 0);
    return bytes;
}

mm6::V4SaveFields exampleFields() {
    mm6::V4SaveFields fields;
    auto &v3 = fields.v3;
    v3.raw.core.party.playingTicks = 42;
    v3.raw.core.party.activeCharacter = 1;
    for (mm6::CoreCharacterFields &character : v3.raw.core.characters)
        character.name = "Hero";
    v3.raw.inventory.ready = true;
    v3.raw.metadata.header.displayName = "V4 full session";
    v3.raw.metadata.header.mapFileName = "outside0.odm";
    v3.raw.metadata.header.playingTicks = 42;
    v3.raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        v3.raw.maps.deltas.push_back({"inside" + std::to_string(index) + ".dlv", indoorDelta()});
    for (int index = 0; index < 15; ++index)
        v3.raw.maps.deltas.push_back({"outside" + std::to_string(index) + ".ddm",
                                      outdoorDelta(index == 0)});
    v3.raw.chest = {17, {}, "outside0.ddm", v3.raw.maps.deltas[52].bytes};
    v3.raw.npcs.activeCount = 399;
    v3.raw.recovery.playingTicks = 42;
    v3.raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    Actor actor(0);
    actor.monsterInfo.id = MONSTER_GOBLIN_A;
    actor.monsterId = MONSTER_INVALID;
    actor.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    actor.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    const std::array<Actor, 1> actors{actor};
    v3.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    v3.generatedNpcs.mapName = "outside0.odm";
    v3.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    v3.gameplayRng.sequentialState = 29;
    mm6::EventRuntimeMapEntry events;
    events.mapName = "outside0.odm";
    events.lastRespawnDay = 1;
    events.mapVars[74] = 9;
    events.decorVars[124] = 7;
    fields.eventRuntime.maps.push_back(events);
    return fields;
}

std::vector<std::string> expectedNames(const mm6::V4SaveFields &fields) {
    std::vector<std::string> names;
    for (const mm6::SavedMapDelta &delta : fields.v3.raw.maps.deltas)
        names.push_back(delta.name);
    return names;
}

Blob baseReader(std::string_view name) {
    const auto bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
    return Blob::copy(bytes.data(), bytes.size());
}

bool validItem(std::int32_t id) { return id >= 1 && id <= 580; }

mm6::NpcStaticBaseline freshNpcs() {
    const auto table = [](std::string_view value) { return Blob::copy(value.data(), value.size()); };
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

mm6::V4FullSessionPreflight stage(const mm6::V4SaveFields &fields,
                                  const mm6::NpcStaticBaseline &npcs,
                                  const ItemTable &items,
                                  std::span<LevelDecoration> decorations,
                                  GameTitle title = GameTitle::MM6) {
    const auto names = expectedNames(fields);
    return mm6::stageV4FullSessionPreflight(
        mm6::encodeV4Save(fields, GameTitle::MM6), npcs, names,
        baseReader, validItem, items, 5, 145, 10,
        RANDOM_ENGINE_SEQUENTIAL, decorations, title);
}

} // namespace

UNIT_TEST(MM6V4FullSessionPreflight, JoinsExactActiveMapAndEventOwner) {
    ItemTableScope items;
    const auto npcs = freshNpcs();
    const auto fields = exampleFields();
    std::vector<LevelDecoration> decorations;
    const auto staged = stage(fields, npcs, *items.table, decorations);
    EXPECT_EQ(staged.v3.actorMap.activeMap.mapName, "outside0.odm");
    EXPECT_EQ(staged.activeEventRuntime.mapName, "outside0.odm");
    EXPECT_EQ(staged.activeEventRuntime.mapVars[74], 9);
    EXPECT_EQ(staged.activeEventRuntime.decorVars[124], 7);
    EXPECT_EQ(staged.saved.eventRuntime, fields.eventRuntime);
}

UNIT_TEST(MM6V4FullSessionPreflight, RefusesStaleHeaderAndDecorationShape) {
    ItemTableScope items;
    const auto npcs = freshNpcs();
    auto fields = exampleFields();
    std::vector<LevelDecoration> decorations;
    fields.eventRuntime.maps[0].lastRespawnDay = 2;
    EXPECT_THROW((void) stage(fields, npcs, *items.table, decorations), std::exception);
    fields = exampleFields();
    decorations.resize(1);
    EXPECT_THROW((void) stage(fields, npcs, *items.table, decorations), std::exception);
    decorations.clear();
    EXPECT_THROW((void) stage(fields, npcs, *items.table, decorations, GameTitle::MM7),
                 std::exception);
}
