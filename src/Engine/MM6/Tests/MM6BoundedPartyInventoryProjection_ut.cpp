#include <cstddef>
#include <cstdint>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BoundedPartyInventoryProjection.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TableScope() {
        pItemTable = table.get();
        const ItemId armor = static_cast<ItemId>(10);
        table->items[armor].iconName = "armor";
        table->items[armor].type = ITEM_TYPE_ARMOUR;
        table->itemSizes[armor] = Sizei(1, 1);
    }

    ~TableScope() { pItemTable = previous; }
};

mm6::RawInventoryFields fourArmors() {
    mm6::RawInventoryFields raw;
    raw.ready = true;
    for (std::size_t i = 0; i < raw.characters.size(); ++i) {
        auto &character = raw.characters[i];
        character.items[137].setWord(0, 10);
        character.items[137].setWord(4, 12);
        character.items[137].setWord(8, 7);
        character.items[137][26] = static_cast<std::uint8_t>(0x80 + i);
        character.items[136][27] = static_cast<std::uint8_t>(0xa0 + i);
        character.equipmentWords[3] = 138;
    }
    return raw;
}

} // namespace

UNIT_TEST(MM6BoundedPartyInventoryProjection, FourCharacterRawRoundTripAndAtomicCapture) {
    TableScope scope;
    const auto source = fourArmors();
    auto projection = mm6::stageBoundedPartyInventory(source, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(projection);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(projection->inventory(i).capacity(), 138);
        EXPECT_EQ(projection->inventory(i).entry(ITEM_SLOT_ARMOUR).index(), 137);
    }
    EXPECT_EQ(mm6::captureBoundedPartyInventory(*projection, *scope.table, GameTitle::MM6), source);

    projection->inventory(2).entry(137)->standardEnchantmentStrength = 19;
    auto captured = mm6::captureBoundedPartyInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(captured);
    EXPECT_EQ(captured->characters[2].items[137].bonusStrengthRaw(), 19);
    EXPECT_EQ(captured->characters[2].items[137][26], 0x82);
    EXPECT_EQ(captured->characters[2].items[136][27], 0xa2);
    EXPECT_EQ(captured->characters[0], source.characters[0]);
    EXPECT_EQ(captured->characters[3], source.characters[3]);

    const auto bytes = mm6::encodeSaveContainer({{mm6::encodeRawInventorySection(*captured)}});
    EXPECT_EQ(mm6::decodeRawInventorySection(mm6::decodeSaveContainer(bytes)), *captured);

    projection->inventory(3).entry(137)->mm6RawRecord.reset();
    EXPECT_FALSE(mm6::captureBoundedPartyInventory(*projection, *scope.table, GameTitle::MM6));
    projection->inventory(3).entry(137)->mm6RawRecord = source.characters[3].items[137];
    Item moved = projection->inventory(3).take(projection->inventory(3).entry(137));
    ASSERT_EQ(projection->inventory(2).tryAdd(Pointi(0, 0), moved).index(), 0);
    auto handedOver = mm6::captureBoundedPartyInventory(*projection, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(handedOver);
    EXPECT_EQ(handedOver->characters[2].items[0], source.characters[3].items[137]);
    EXPECT_EQ(handedOver->characters[2].gridWords[0], 1);
    EXPECT_EQ(handedOver->characters[3].items[137].itemNumberRaw(), 0);
    EXPECT_EQ(handedOver->characters[3].items[137][26], 0x83);
    EXPECT_EQ(handedOver->characters[3].equipmentWords[3], 0);
    EXPECT_EQ(source, fourArmors());
}

UNIT_TEST(MM6BoundedPartyInventoryProjection, RejectsLateMalformedCharacterAndForeignTitle) {
    TableScope scope;
    auto source = fourArmors();
    source.characters[3].equipmentWords[3] = 139;
    const auto prior = source;
    EXPECT_FALSE(mm6::stageBoundedPartyInventory(source, *scope.table, GameTitle::MM6));
    EXPECT_EQ(source, prior);

    source = fourArmors();
    EXPECT_FALSE(mm6::stageBoundedPartyInventory(source, *scope.table, GameTitle::MM7));
    source.ready = false;
    EXPECT_FALSE(mm6::stageBoundedPartyInventory(source, *scope.table, GameTitle::MM6));
}
