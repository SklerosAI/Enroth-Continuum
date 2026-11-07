#include <cstdint>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BackpackItemEventRemoval.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    ItemTable table;

    TableScope() {
        pItemTable = &table;
        const ItemId armor = static_cast<ItemId>(10);
        table.items[armor].iconName = "armor";
        table.items[armor].type = ITEM_TYPE_ARMOUR;
        table.itemSizes[armor] = Sizei(2, 2);
        const ItemId scroll = static_cast<ItemId>(221);
        table.items[scroll].iconName = "scroll";
        table.items[scroll].type = ITEM_TYPE_SPELL_SCROLL;
        table.itemSizes[scroll] = Sizei(1, 1);
    }
    ~TableScope() { pItemTable = previous; }
};

mm6::ItemRecord28 record(std::uint32_t id, std::uint8_t tail = 0x5a) {
    mm6::ItemRecord28 result;
    result.setWord(0, id);
    result.setWord(20, 1);
    result[25] = 0x6b;
    result[26] = tail;
    result[27] = 0xa5;
    return result;
}

mm6::RawInventoryFields fixture() {
    mm6::RawInventoryFields raw;
    raw.ready = true;
    raw.characters[0].items[137] = record(10);
    raw.characters[0].gridWords[0] = 138;
    for (int cell : {1, 14, 15})
        raw.characters[0].gridWords[cell] = std::uint32_t(-1);
    raw.characters[1].items[17][26] = 0xe1; // Unreferenced zero-ID tail.
    raw.characters[3].items[0] = record(221, 0x77);
    raw.characters[3].gridWords[8] = 1;
    return raw;
}

CharacterInventory runtimeFrom(const mm6::RawCharacterInventory &raw, const ItemTable &table) {
    auto projection = mm6::stageBoundedInventory(raw, table, GameTitle::MM6);
    if (!projection)
        throw std::runtime_error("invalid synthetic inventory");
    return projection->inventory();
}

} // namespace

UNIT_TEST(MM6BackpackItemEventRemoval, ClearsGridAndPrefixButRetainsPhysicalTail) {
    TableScope scope;
    auto raw = fixture();
    const auto untouchedCharacter = raw.characters[3];
    const auto untouchedZeroIdTail = raw.characters[1].items[17];
    CharacterInventory inventory = runtimeFrom(raw.characters[0], scope.table);

    auto removed = mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 0, 0, inventory, scope.table, GameTitle::MM6);
    ASSERT_TRUE(removed);
    EXPECT_EQ(*removed, record(10));
    EXPECT_EQ(inventory.size(), 0);
    for (int byte = 0; byte < 25; ++byte)
        EXPECT_EQ(raw.characters[0].items[137][byte], 0) << byte;
    for (int byte = 25; byte < 28; ++byte)
        EXPECT_EQ(raw.characters[0].items[137][byte], (*removed)[byte]) << byte;
    for (int cell : {0, 1, 14, 15})
        EXPECT_EQ(raw.characters[0].gridWords[cell], 0u);
    EXPECT_EQ(raw.characters[3], untouchedCharacter);
    EXPECT_EQ(raw.characters[1].items[17], untouchedZeroIdTail);
    auto replay = mm6::stageBoundedPartyInventory(raw, scope.table, GameTitle::MM6);
    ASSERT_TRUE(replay);
    EXPECT_EQ(replay->inventory(0).size(), 0);
}

UNIT_TEST(MM6BackpackItemEventRemoval, RejectsWrongAnchorTitleAndLiveDrift) {
    TableScope scope;
    auto raw = fixture();
    const auto original = raw;
    CharacterInventory inventory = runtimeFrom(raw.characters[0], scope.table);
    EXPECT_FALSE(mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 0, 1, inventory, scope.table, GameTitle::MM6));
    EXPECT_FALSE(mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 0, 0, inventory, scope.table, GameTitle::MM7));
    EXPECT_FALSE(mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 4, 0, inventory, scope.table, GameTitle::MM6));
    EXPECT_FALSE(mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 0, 126, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(raw, original);
    EXPECT_EQ(inventory.size(), 1);

    auto carried = mm6::stageBoundedItem(record(221), scope.table, GameTitle::MM6);
    ASSERT_TRUE(carried);
    ASSERT_TRUE(inventory.tryAdd(mm6::boundedItem(*carried)));
    EXPECT_FALSE(mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 0, 0, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(raw, original);
    EXPECT_EQ(inventory.size(), 2);
}

UNIT_TEST(MM6BackpackItemEventRemoval, RejectsMalformedOtherCharacterBeforeMutation) {
    TableScope scope;
    auto raw = fixture();
    raw.characters[3].gridWords[9] = 139; // Late physical owner is malformed.
    const auto original = raw;
    CharacterInventory inventory = runtimeFrom(raw.characters[0], scope.table);
    EXPECT_FALSE(mm6::tryRemovePhysicalBackpackItemForEvent(
        raw, 0, 0, inventory, scope.table, GameTitle::MM6));
    EXPECT_EQ(raw, original);
    EXPECT_EQ(inventory.size(), 1);
}
