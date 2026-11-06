#include <cstdint>
#include <memory>
#include <span>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/Tables/ChestTable.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TableScope() {
        pItemTable = table.get();
        table->itemSizes[static_cast<ItemId>(200)] = Sizei(1, 1);
        initializeChests();
    }
    ~TableScope() { pItemTable = previous; }
};

mm6::ChestRecord oneItemRecord() {
    mm6::ChestRecord record{};
    mm6::writeChest32(std::span(record).subspan(4, 28), 0, 200);
    mm6::writeChest32(std::span(record).subspan(4, 28), 20, 1);
    record[4 + 26] = 0x5a;  // MM6 opaque item byte must survive runtime projection.
    return record;
}

}  // namespace

UNIT_TEST(MM6ChestRuntimeProjection, ResolvedItemAndFirstOpenGridRoundTripWithOpaqueTail) {
    TableScope table;
    const auto source = oneItemRecord();
    auto projected = mm6::stageChestRuntimeProjection(GameTitle::MM6, source, 0);
    ASSERT_TRUE(projected);
    EXPECT_EQ(projected->chest.inventory.size(), 1);
    auto before = mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected);
    ASSERT_TRUE(before);
    EXPECT_EQ(*before, source);

    Item item = projected->chest.inventory.take(projected->chest.inventory.entry(0));
    projected->chest.inventory.add(Pointi(0, 0), item);
    projected->chest.SetInitialized(true);
    auto changed = mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected);
    ASSERT_TRUE(changed);
    EXPECT_EQ(mm6::readChest16(*changed, 2) & 2u, 2u);
    EXPECT_EQ(mm6::readChest16(*changed, mm6::CHEST_GRID_OFFSET), 1u);
    EXPECT_EQ((*changed)[4 + 26], 0x5a);
    auto reopened = mm6::stageChestRuntimeProjection(GameTitle::MM6, *changed, 0);
    ASSERT_TRUE(reopened);
    auto again = mm6::stageChestRuntimeRecord(GameTitle::MM6, *reopened);
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *changed);
}

UNIT_TEST(MM6ChestRuntimeProjection, RejectsForeignMarkerAndInvalidGridWithoutMutation) {
    TableScope table;
    const auto source = oneItemRecord();
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM7, source, 0));
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM6, source, 20));
    auto marker = source;
    mm6::writeChest32(std::span(marker).subspan(4, 28), 0, -1);
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM6, marker, 0));
    auto badGrid = source;
    mm6::writeChest16(badGrid, mm6::CHEST_GRID_OFFSET, 82);
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM6, badGrid, 0));
    auto tailGrid = source;
    mm6::writeChest16(tailGrid, mm6::CHEST_GRID_OFFSET + 2 * 139, 1);
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM6, tailGrid, 0));
    EXPECT_EQ(source, oneItemRecord());
}
