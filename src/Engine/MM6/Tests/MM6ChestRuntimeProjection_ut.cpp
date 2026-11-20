#include <cstdint>
#include <memory>
#include <span>
#include <vector>

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
        table->itemSizes[ITEM_GOLD_SMALL] = Sizei(1, 1);
        table->itemSizes[static_cast<ItemId>(400)] = Sizei(1, 1);
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
    ASSERT_TRUE(projected->chest.inventory.entry(0)->mm6RawRecord);
    EXPECT_EQ((*projected->chest.inventory.entry(0)->mm6RawRecord)[26], 0x5a);
    auto before = mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected);
    ASSERT_TRUE(before);
    EXPECT_EQ(*before, source);

    Item item = projected->chest.inventory.take(projected->chest.inventory.entry(0));
    ASSERT_TRUE(item.mm6RawRecord);
    EXPECT_EQ((*item.mm6RawRecord)[26], 0x5a);
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

UNIT_TEST(MM6ChestRuntimeProjection, RejectsLostChangedOrWrongSlotProvenance) {
    TableScope table;
    const auto source = oneItemRecord();
    auto projected = mm6::stageChestRuntimeProjection(GameTitle::MM6, source, 0);
    ASSERT_TRUE(projected);

    projected->chest.inventory.entry(0)->mm6RawRecord.reset();
    EXPECT_FALSE(mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected));

    projected = mm6::stageChestRuntimeProjection(GameTitle::MM6, source, 0);
    ASSERT_TRUE(projected);
    (*projected->chest.inventory.entry(0)->mm6RawRecord)[26] ^= 1;
    EXPECT_FALSE(mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected));

    projected = mm6::stageChestRuntimeProjection(GameTitle::MM6, source, 0);
    ASSERT_TRUE(projected);
    projected->chest.inventory.entry(0)->numCharges++;
    EXPECT_FALSE(mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected));
    EXPECT_EQ(source, oneItemRecord());
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

UNIT_TEST(MM6ChestRuntimeProjection, LaterPageItemIsStashedAndMustBeOnOnePage) {
    // An item on the second page projects into the stash with its page cell. An item that is also on the first page, or
    // that lies over another item of its page, doesn't project.
    TableScope table;
    auto paged = oneItemRecord();
    mm6::writeChest16(paged, 2, 2);
    mm6::writeChest32(std::span(paged).subspan(4 + 28, 28), 0, 400);
    mm6::writeChest16(paged, mm6::CHEST_GRID_OFFSET, 1);
    mm6::writeChest16(paged, mm6::CHEST_GRID_OFFSET + 2 * 81, 1 + 1 + 140 * 10);
    auto projected = mm6::stageChestRuntimeProjection(GameTitle::MM6, paged, 0);
    ASSERT_TRUE(projected);
    EXPECT_EQ(projected->chest.mm6Pages, (std::vector<mm6::ChestPageItem>{{1, 1, 10}}));
    EXPECT_EQ(projected->chest.inventory.entry(1).zone(), INVENTORY_ZONE_STASH);
    EXPECT_EQ(mm6::chestPageEntry(projected->chest, 1, Pointi(1, 1)).index(), 1);
    EXPECT_FALSE(mm6::chestPageEntry(projected->chest, 0, Pointi(1, 1)));
    EXPECT_EQ(mm6::stageChestRuntimeRecord(GameTitle::MM6, *projected), paged);

    auto twice = paged;
    mm6::writeChest16(twice, mm6::CHEST_GRID_OFFSET + 2 * 81, 1 + 0 + 140 * 10);
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM6, twice, 0));
    auto overlapping = paged;
    mm6::writeChest32(std::span(overlapping).subspan(4 + 2 * 28, 28), 0, 200);
    mm6::writeChest16(overlapping, mm6::CHEST_GRID_OFFSET + 2 * 82, 1 + 2 + 140 * 10);
    EXPECT_FALSE(mm6::stageChestRuntimeProjection(GameTitle::MM6, overlapping, 0));
}

UNIT_TEST(MM6ChestRuntimeProjection, GoldAndSafeArtifactKeepTheirItemFields) {
    TableScope table;
    auto gold = oneItemRecord();
    mm6::writeChest32(std::span(gold).subspan(4, 28), 0, std::to_underlying(ITEM_GOLD_SMALL));
    mm6::writeChest32(std::span(gold).subspan(4, 28), 12, 79);
    auto goldProjection = mm6::stageChestRuntimeProjection(GameTitle::MM6, gold, 0);
    ASSERT_TRUE(goldProjection);
    auto goldWire = mm6::stageChestRuntimeRecord(GameTitle::MM6, *goldProjection);
    ASSERT_TRUE(goldWire);
    EXPECT_EQ(*goldWire, gold);

    auto artifact = oneItemRecord();
    mm6::writeChest32(std::span(artifact).subspan(4, 28), 0, 400);
    auto artifactProjection = mm6::stageChestRuntimeProjection(GameTitle::MM6, artifact, 0);
    ASSERT_TRUE(artifactProjection);
    auto artifactWire = mm6::stageChestRuntimeRecord(GameTitle::MM6, *artifactProjection);
    ASSERT_TRUE(artifactWire);
    EXPECT_EQ(*artifactWire, artifact);
}
