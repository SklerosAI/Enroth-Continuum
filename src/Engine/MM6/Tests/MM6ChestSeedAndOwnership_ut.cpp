#include <cstdint>
#include <memory>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestBaseItemPicker.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/Tables/ItemTable.h"

TEST(MM6ChestSeedAndOwnership, NewGameSeedRequiresExplicitMM6Source) {
    mm6::ChestTitleState owner;
    owner.lcgState = 19;
    EXPECT_FALSE(mm6::initializeNewGameChestTitleState(
        GameTitle::MM7, &owner, 8, mm6::NewGameChestSeedSource::ExplicitDeterministic));
    EXPECT_FALSE(mm6::initializeNewGameChestTitleState(
        GameTitle::MM6, &owner, 8, mm6::NewGameChestSeedSource::Unverified));
    EXPECT_FALSE(mm6::initializeNewGameChestTitleState(
        GameTitle::MM6, nullptr, 8, mm6::NewGameChestSeedSource::ExplicitDeterministic));
    EXPECT_FALSE(owner.ready);
    EXPECT_EQ(owner.lcgState, 19u);

    owner.generatedArtifactFlags[0] = 1;
    ASSERT_TRUE(mm6::initializeNewGameChestTitleState(
        GameTitle::MM6, &owner, 0, mm6::NewGameChestSeedSource::ExplicitDeterministic));
    EXPECT_TRUE(owner.ready);
    EXPECT_EQ(owner.lcgState, 0u);  // Zero is a valid 32-bit LCG state.
    for (std::uint8_t flag : owner.generatedArtifactFlags)
        EXPECT_EQ(flag, 0);
    EXPECT_EQ(mm6::nextChestRandom15(owner.lcgState), 38u);
}

TEST(MM6ChestSeedAndOwnership, ScansFourInventoriesEquipmentAndHeldItem) {
    auto table = std::make_unique<ItemTable>();
    ItemTable *previousTable = pItemTable;
    pItemTable = table.get();
    for (int id : {400, 401, 402, 403, 429, 430})
        table->itemSizes[static_cast<ItemId>(id)] = Sizei(1, 1);

    auto party = std::make_unique<Party>();
    Item first;
    first.itemId = static_cast<ItemId>(400);
    EXPECT_TRUE(!!party->pCharacters[0].inventory.tryAdd(first));
    Item equipped;
    equipped.itemId = static_cast<ItemId>(401);
    EXPECT_TRUE(!!party->pCharacters[1].inventory.tryEquip(ITEM_SLOT_RING1, equipped));
    Item middle;
    middle.itemId = static_cast<ItemId>(402);
    EXPECT_TRUE(!!party->pCharacters[2].inventory.tryAdd(middle));
    Item last;
    last.itemId = static_cast<ItemId>(429);
    EXPECT_TRUE(!!party->pCharacters[3].inventory.tryAdd(last));
    party->pPickedItem.itemId = static_cast<ItemId>(403);
    auto owned = mm6::scanPartyOwnedChestArtifacts(GameTitle::MM6, *party);
    ASSERT_TRUE(owned);
    EXPECT_EQ((*owned)[0], 1);
    EXPECT_EQ((*owned)[1], 1);
    EXPECT_EQ((*owned)[2], 1);
    EXPECT_EQ((*owned)[3], 1);
    EXPECT_EQ((*owned)[29], 1);
    EXPECT_FALSE(mm6::scanPartyOwnedChestArtifacts(GameTitle::MM7, *party));

    party->pPickedItem.itemId = static_cast<ItemId>(430);
    owned = mm6::scanPartyOwnedChestArtifacts(GameTitle::MM6, *party);
    ASSERT_TRUE(owned);
    EXPECT_EQ((*owned)[3], 0);
    EXPECT_EQ((*owned)[29], 1);
    pItemTable = previousTable;
}
