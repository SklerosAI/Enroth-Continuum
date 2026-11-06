#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/ChestTable.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();
    TableScope() {
        pItemTable = table.get();
        initializeChests();
        for (int id = 1; id <= 580; ++id)
            table->itemSizes[static_cast<ItemId>(id)] = Sizei(1, 1);
        table->items[static_cast<ItemId>(1)].type = ITEM_TYPE_RING;
        auto &item = table->items[static_cast<ItemId>(200)];
        item.type = ITEM_TYPE_RING;
        for (int tier = 1; tier <= 6; ++tier)
            item.uChanceByTreasureLvl[static_cast<ItemTreasureLevel>(tier)] = 100;
        for (auto tier : table->standardEnchantmentRangeByTreasureLevel.indices())
            table->standardEnchantmentRangeByTreasureLevel[tier] = {1, 1};
    }
    ~TableScope() { pItemTable = previous; }
};

void put32(std::vector<std::uint8_t> *bytes, size_t offset, std::uint32_t value) {
    for (int index = 0; index < 4; ++index)
        (*bytes)[offset + index] = static_cast<std::uint8_t>(value >> (index * 8));
}

std::vector<std::uint8_t> sidecar(int marker = -1) {
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    std::vector<std::uint8_t> bytes(chestOffset + 20 * mm6::CHEST_WIRE_SIZE + 200 + 56);
    put32(&bytes, chestOffset - 4, 20);
    put32(&bytes, chestOffset + 4, static_cast<std::uint32_t>(marker));
    bytes[0] = 3;
    return bytes;
}

std::vector<Chest> baselineChests(const std::vector<std::uint8_t> &delta) {
    const auto wire = mm6::decodeOutdoorDelta(delta);
    mm6::ChestRecordSet records{};
    EXPECT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    std::vector<Chest> chests(20);
    for (int index = 0; index < 20; ++index) {
        const Chest_MM7 source = projectMM6ChestRecord(records[index]);
        reconstruct(source, &chests[index], tags::context<int>(index));
    }
    return chests;
}

}  // namespace

UNIT_TEST(MM6ChestMapLoadRoute, ScopedBindingCommitsAllOwnersOnlyWhenReady) {
    TableScope table;
    auto delta = sidecar();
    auto chests = baselineChests(delta);
    const auto before = delta;
    mm6::ChestTitleState state;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    auto party = std::make_unique<Party>();
    mm6::ChestLiveBinding binding;
    binding.title = GameTitle::MM6;
    binding.levelType = LEVEL_OUTDOOR;
    binding.chests = &chests;
    binding.deltaBytes = &delta;
    binding.titleState = &state;

    EXPECT_FALSE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                           table.table.get(), party.get(), binding));
    EXPECT_EQ(delta, before);
    EXPECT_EQ(state.lcgState, 8u);
    {
        mm6::ScopedChestMapLoadOptIn optIn;
        ASSERT_TRUE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                               table.table.get(), party.get(), binding));
    }
    EXPECT_NE(delta, before);
    EXPECT_NE(state.lcgState, 8u);
    EXPECT_EQ(chests.size(), 20u);
    EXPECT_FALSE(mm6::chestMapLoadOptedIn());
}

UNIT_TEST(MM6ChestMapLoadRoute, ForeignMissingAndMalformedInputsLeaveOwnersUntouched) {
    TableScope table;
    auto delta = sidecar();
    auto chests = baselineChests(delta);
    const auto before = delta;
    mm6::ChestTitleState state;
    auto party = std::make_unique<Party>();
    mm6::ChestLiveBinding binding;
    binding.title = GameTitle::MM6;
    binding.levelType = LEVEL_OUTDOOR;
    binding.chests = &chests;
    binding.deltaBytes = &delta;
    binding.titleState = &state;
    mm6::ScopedChestMapLoadOptIn optIn;
    EXPECT_FALSE(mm6::tryChestMapLoadBinding(GameTitle::MM7, MAP_TREASURE_LEVEL_1,
                                           table.table.get(), party.get(), binding));
    EXPECT_FALSE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                           nullptr, party.get(), binding));
    EXPECT_FALSE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                           table.table.get(), nullptr, binding));
    EXPECT_FALSE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                           table.table.get(), party.get(), binding));
    EXPECT_EQ(delta, before);
    EXPECT_EQ(state.lcgState, 0u);
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    delta.pop_back();  // Truncated sidecar cannot describe its fixed-width tail.
    const auto malformed = delta;
    EXPECT_FALSE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                           table.table.get(), party.get(), binding));
    EXPECT_EQ(delta, malformed);
    EXPECT_EQ(state.lcgState, 8u);
}

UNIT_TEST(MM6ChestMapLoadRoute, PartyOwnedArtifactCapIsAppliedAtMapEntry) {
    TableScope table;
    auto delta = sidecar(-7);
    auto chests = baselineChests(delta);
    mm6::ChestTitleState state;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    auto party = std::make_unique<Party>();
    for (int id = 400; id < 413; ++id) {
        Item item;
        item.itemId = static_cast<ItemId>(id);
        ASSERT_TRUE(!!party->pCharacters[0].inventory.tryAdd(item));
    }
    mm6::ChestLiveBinding binding;
    binding.title = GameTitle::MM6;
    binding.levelType = LEVEL_OUTDOOR;
    binding.chests = &chests;
    binding.deltaBytes = &delta;
    binding.titleState = &state;
    mm6::ScopedChestMapLoadOptIn optIn;
    ASSERT_TRUE(mm6::tryChestMapLoadBinding(GameTitle::MM6, MAP_TREASURE_LEVEL_1,
                                           table.table.get(), party.get(), binding));
    const auto wire = mm6::decodeOutdoorDelta(delta);
    mm6::ChestRecordSet records{};
    ASSERT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    EXPECT_EQ(mm6::readChest32(std::span(records[0]).subspan(4, 28), 0), 0);
    for (std::uint8_t generated : state.generatedArtifactFlags)
        EXPECT_EQ(generated, 0);
}
