#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawNewGameSaveRuntime.h"
#include "Engine/MM6/MM6PlainItemProjection.h"
#include "Engine/Tables/ItemTable.h"

#ifndef _WIN32
#include <cstdlib>
#endif

namespace {

struct ItemTableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    ItemTableScope() {
        pItemTable = table.get();
        const ItemId id = static_cast<ItemId>(10);
        table->items[id].iconName = "source-item";
        table->items[id].type = static_cast<ItemType>(3);
        table->items[id].rarity = RARITY_COMMON;
        table->itemSizes[id] = Sizei(1, 1);
    }
    ~ItemTableScope() { pItemTable = previous; }
};

std::vector<std::string> names() {
    std::vector<std::string> result;
    for (int i = 0; i < 52; ++i)
        result.push_back("d" + std::to_string(i) + ".dlv");
    result.push_back("oute3.ddm");
    for (int i = 1; i < 15; ++i)
        result.push_back("o" + std::to_string(i) + ".ddm");
    return result;
}

std::vector<std::uint8_t> delta(std::string_view name) {
    if (name.ends_with(".dlv"))
        return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

mm6::DefaultDeltaReader defaultReader() {
    return [](std::string_view name) {
        const auto bytes = delta(name);
        return Blob::copy(bytes.data(), bytes.size());
    };
}

mm6::MapBaseReader baseReader() {
    return [](std::string_view name) {
        std::vector<std::uint8_t> bytes = name.ends_with(".blv")
            ? std::vector<std::uint8_t>(136 + 10 * 4, 0)
            : std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
        if (name.ends_with(".blv"))
            bytes[136 + 4 * 4] = 200;
        return Blob::copy(bytes.data(), bytes.size());
    };
}

Party sourceParty() {
    Party party;
    for (Character &character : party.pCharacters)
        character.name = "Hero";
    party.pPickedItem.Reset();
    return party;
}

mm6::KnownSaveFields seed(const Party &party) {
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    mm6::KnownSaveFields result = mm6::captureFirstMapKnownSeed(
        party, npcs, timer, overlays, "First map", 5, GameTitle::MM6);
    result.chest.lcgState = 17;
    return result;
}

mm6::PreparedRawNewGameAutosave prepare(
    const mm6::KnownSaveFields &known, const Party &party,
    const mm6::RawInventoryFields &inventory,
    const mm6::RawHeldCursorOwner &cursor, const mm6::ShopTitleState &shops,
    const ItemTable &table, GameTitle title = GameTitle::MM6) {
    const auto expected = names();
    return mm6::prepareOfflineRawNewGameAutosave(
        known, party, inventory, cursor, shops, {}, expected,
        defaultReader(), baseReader(),
        [](std::int32_t id) { return id >= 1 && id <= 580; }, table, title);
}

} // namespace

UNIT_TEST(MM6RawNewGameSaveRuntime, PreservesPhysicalOwnersInV2FirstMapCandidate) {
    ItemTableScope scope;
    const Party party = sourceParty();
    const auto known = seed(party);
    mm6::RawInventoryFields inventory;
    inventory.ready = true;
    inventory.characters[0].items[137][27] = 0xa5; // Unreferenced raw tail remains exact.
    auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    cursor.record[27] = 0xc7; // Empty cursor retains its opaque trailing byte.
    const auto shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);

    auto prepared = prepare(known, party, inventory, cursor, shops, *scope.table);
    const auto decoded = mm6::decodeRawSave(prepared.bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.inventory, inventory);
    EXPECT_EQ(decoded.heldItem.record, cursor.record);
    EXPECT_EQ(decoded.shops, shops);
    EXPECT_EQ(decoded.maps.deltas.size(), 67);
    EXPECT_EQ(decoded.metadata.header.mapFileName, "oute3.odm");
    EXPECT_EQ(decoded.chest.rawActiveDelta, delta("oute3.ddm"));
    EXPECT_EQ(prepared.staged.saved.inventory, inventory);
    EXPECT_EQ(prepared.staged.saved.heldItem, decoded.heldItem);
    EXPECT_EQ(inventory.characters[0].items[137][27], 0xa5);
    EXPECT_EQ(cursor.record[27], 0xc7);

    EXPECT_THROW((void) prepare(known, party, inventory, cursor, shops, *scope.table,
                                GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6RawNewGameSaveRuntime, RejectsMismatchedOrUnsupportedSourceItems) {
    ItemTableScope scope;
    Party party = sourceParty();
    auto known = seed(party);
    mm6::RawInventoryFields inventory;
    inventory.ready = true;
    auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    auto shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);

    inventory.characters[0].items[0].setWord(0, 10);
    inventory.characters[0].gridWords[0] = 1;
    EXPECT_THROW((void) prepare(known, party, inventory, cursor, shops, *scope.table), std::exception);
    Party withoutRawRecord = sourceParty();
    Item ordinaryStarter;
    ordinaryStarter.itemId = static_cast<ItemId>(10);
    ASSERT_TRUE(withoutRawRecord.pCharacters[0].inventory.tryRestoreAt(
        0, Pointi(0, 0), ordinaryStarter));
    const auto ordinarySeed = seed(withoutRawRecord);
    EXPECT_THROW((void) prepare(ordinarySeed, withoutRawRecord, inventory,
                                cursor, shops, *scope.table), std::exception);
    inventory.characters[0].gridWords[0] = 0;
    cursor.record.setWord(0, 10);
    EXPECT_THROW((void) prepare(known, party, inventory, cursor, shops, *scope.table), std::exception);
    cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    shops.clocks.deadlineWords[0] = 1;
    EXPECT_THROW((void) prepare(known, party, inventory, cursor, shops, *scope.table), std::exception);
    shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);

    // The Party carries the same raw item, but its potion union is outside the
    // currently reversible plain/standard/Antique projection.
    mm6::ItemRecord28 unsupported;
    unsupported.setWord(0, 10);
    unsupported.setWord(12, 1);
    scope.table->items[static_cast<ItemId>(10)].type = ITEM_TYPE_POTION;
    Item item;
    item.itemId = static_cast<ItemId>(10);
    item.mm6RawRecord = unsupported;
    ASSERT_TRUE(party.pCharacters[0].inventory.tryRestoreAt(0, Pointi(0, 0), item));
    inventory.characters[0].items[0] = unsupported;
    inventory.characters[0].gridWords[0] = 1;
    known = seed(party);
    EXPECT_THROW((void) prepare(known, party, inventory, cursor, shops, *scope.table), std::exception);
    EXPECT_EQ(inventory.characters[0].items[0], unsupported);
}

UNIT_TEST(MM6RawNewGameSaveRuntime, WritesMatchedPlainItemAndCursorAsNativeV2) {
#ifdef _WIN32
    GTEST_SKIP() << "Native test profile uses mkdtemp";
#else
    ItemTableScope scope;
    Party party = sourceParty();
    mm6::RawInventoryFields inventory;
    inventory.ready = true;
    mm6::ItemRecord28 record;
    record.setWord(0, 10);
    record[27] = 0x9a;
    auto projected = mm6::stagePlainItem(record, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projected);
    ASSERT_TRUE(party.pCharacters[0].inventory.tryRestoreAt(
        5, Pointi(0, 0), projected->item));
    inventory.characters[0].items[5] = record;
    inventory.characters[0].gridWords[0] = 6;
    party.pPickedItem = projected->item;
    auto cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    cursor.record = record;
    const auto shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    const auto known = seed(party);
    auto prepared = prepare(known, party, inventory, cursor, shops, *scope.table);
    EXPECT_EQ(prepared.staged.saved.inventory.characters[0].items[5], record);
    EXPECT_EQ(prepared.staged.saved.heldItem.record, record);

    std::string pattern = (std::filesystem::temp_directory_path() / "mm6-raw-newgame-XXXXXX").string();
    ASSERT_TRUE(::mkdtemp(pattern.data()));
    const std::filesystem::path profile(pattern);
    try {
        mm6::writeOfflineRawNewGameAutosave(profile, prepared, GameTitle::MM6);
        const auto slot = profile / "saves" / "autosave.ec6";
        std::ifstream stream(slot, std::ios::binary);
        ASSERT_TRUE(stream);
        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        EXPECT_EQ(bytes, prepared.bytes);
        EXPECT_EQ(mm6::decodeRawSave(bytes, GameTitle::MM6).inventory, inventory);
        EXPECT_THROW(mm6::writeOfflineRawNewGameAutosave(
            profile, prepared, GameTitle::MM7), std::exception);
        prepared.staged.saved.core.party.gold = 1;
        EXPECT_THROW(mm6::writeOfflineRawNewGameAutosave(
            profile, prepared, GameTitle::MM6), std::exception);
        std::ifstream unchanged(slot, std::ios::binary);
        const std::vector<std::uint8_t> after{
            std::istreambuf_iterator<char>(unchanged), std::istreambuf_iterator<char>()};
        EXPECT_EQ(after, bytes);
    } catch (...) {
        std::filesystem::remove_all(profile);
        throw;
    }
    std::filesystem::remove_all(profile);
#endif
}
