#include <array>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6CircusPrizeEvent.h"
#include "Engine/Objects/Character.h"
#include "Engine/Tables/ItemTable.h"

UNIT_TEST(MM6CircusPrizeEvent, WeightsPrizesAcrossBackpacksAndEquipment) {
    // MM6 variable 224 scores circus prizes 1, 3 and 5 across the whole party, equipped ones included.
    ItemTable localTable;
    ItemTable *previous = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previous};
    pItemTable = &localTable;
    for (int id : {470, 471, 477, 478})
        localTable.itemSizes[static_cast<ItemId>(id)] = Sizei(1, 1);

    auto item = [](int id) {
        Item result;
        result.itemId = static_cast<ItemId>(id);
        return result;
    };

    std::array<Character, 4> characters;
    EXPECT_EQ(mm6::circusPrizeScore(characters), 0);
    ASSERT_TRUE(characters[0].inventory.tryAdd(Pointi(0, 0), item(470)));
    ASSERT_TRUE(characters[0].inventory.tryAdd(Pointi(1, 0), item(470)));
    ASSERT_TRUE(characters[1].inventory.tryAdd(Pointi(0, 0), item(471)));
    ASSERT_TRUE(characters[3].inventory.equip(ITEM_SLOT_RING1, item(477)));
    ASSERT_TRUE(characters[2].inventory.tryAdd(Pointi(0, 0), item(478)));
    EXPECT_EQ(mm6::circusPrizeScore(characters), 10);
}
