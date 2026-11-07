#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CreationItemSeed.h"
#include "Engine/MM6/MM6RawNewGameSaveRuntime.h"

namespace {

struct ItemTableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    ItemTableScope() {
        pItemTable = table.get();
        for (ItemId id : {static_cast<ItemId>(10), static_cast<ItemId>(15)}) {
            table->items[id].iconName = "synthetic";
            table->items[id].rarity = RARITY_COMMON;
            table->itemSizes[id] = Sizei(2, 2);
        }
        table->items[static_cast<ItemId>(10)].type = ITEM_TYPE_RING;
        table->items[static_cast<ItemId>(15)].type = ITEM_TYPE_SINGLE_HANDED;
    }

    ~ItemTableScope() { pItemTable = previous; }
};

} // namespace

UNIT_TEST(MM6CreationItemSeed, CanonicalStarterRecordsRetainPhysicalIndicesAndReplay) {
    ItemTableScope scope;
    Party party;
    party.pPickedItem.Reset();
    party.pCharacters[0].inventory = CharacterInventory::forMM6();
    Item ring(static_cast<ItemId>(10));
    ring.standardEnchantment = ATTRIBUTE_MIGHT;
    ring.standardEnchantmentStrength = 3;
    ring.flags = ITEM_IDENTIFIED;
    ASSERT_TRUE(party.pCharacters[0].inventory.tryRestoreAt(137, Pointi(3, 2), ring));
    Item blade(static_cast<ItemId>(15));
    blade.flags = ITEM_IDENTIFIED;
    ASSERT_TRUE(party.pCharacters[1].inventory.tryRestoreAt(4, Pointi(0, 0), blade));

    auto seeded = mm6::seedMM6CreationItems(party, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(seeded);
    EXPECT_EQ(seeded->inventory.characters[0].items[137].itemNumberRaw(), 10u);
    EXPECT_EQ(seeded->inventory.characters[0].items[137].bonusRaw(), 1u);
    EXPECT_EQ(seeded->inventory.characters[0].items[137].bonusStrengthRaw(), 3u);
    EXPECT_EQ(seeded->inventory.characters[0].gridWords[2 * 14 + 3], 138u);
    EXPECT_EQ(seeded->inventory.characters[0].gridWords[2 * 14 + 4],
              std::bit_cast<std::uint32_t>(-(2 * 14 + 3) - 1));
    EXPECT_EQ(seeded->inventory.characters[1].items[4].itemNumberRaw(), 15u);
    EXPECT_EQ(seeded->party.pCharacters[0].inventory.entry(137)->mm6RawRecord,
              seeded->inventory.characters[0].items[137]);
    EXPECT_EQ(mm6::captureRawPartyInventories(
        {seeded->party, seeded->inventory}, *scope.table, GameTitle::MM6),
        seeded->inventory);
    EXPECT_EQ(seeded->cursor, *mm6::freshRawHeldCursorOwner(GameTitle::MM6));
    EXPECT_FALSE(party.pCharacters[0].inventory.entry(137)->mm6RawRecord);
}

UNIT_TEST(MM6CreationItemSeed, RejectsUnrepresentedAndForeignCreationOwners) {
    ItemTableScope scope;
    Party party;
    party.pPickedItem.Reset();
    Item ring(static_cast<ItemId>(10));
    ring.flags = ITEM_IDENTIFIED;
    ASSERT_TRUE(party.pCharacters[0].inventory.tryRestoreAt(0, Pointi(0, 0), ring));
    EXPECT_FALSE(mm6::seedMM6CreationItems(party, *scope.table, GameTitle::MM7));
    ItemTable foreign;
    EXPECT_FALSE(mm6::seedMM6CreationItems(party, foreign, GameTitle::MM6));

    party.pCharacters[0].inventory.entry(0)->specialEnchantment =
        static_cast<ItemEnchantment>(2);
    EXPECT_FALSE(mm6::seedMM6CreationItems(party, *scope.table, GameTitle::MM6));
    party.pCharacters[0].inventory.entry(0)->specialEnchantment = ITEM_ENCHANTMENT_NULL;
    party.pCharacters[0].inventory.entry(0)->mm6RawRecord = mm6::ItemRecord28{};
    EXPECT_FALSE(mm6::seedMM6CreationItems(party, *scope.table, GameTitle::MM6));
    party.pCharacters[0].inventory.entry(0)->mm6RawRecord.reset();
    party.pPickedItem = ring;
    EXPECT_FALSE(mm6::seedMM6CreationItems(party, *scope.table, GameTitle::MM6));
}

UNIT_TEST(MM6CreationItemSeed, SupportedStarterComposesIntoNativeV2Candidate) {
    ItemTableScope scope;
    Party party;
    party.pPickedItem.Reset();
    for (Character &character : party.pCharacters)
        character.name = "Hero";
    Item ring(static_cast<ItemId>(10));
    ring.flags = ITEM_IDENTIFIED;
    ASSERT_TRUE(party.pCharacters[0].inventory.tryRestoreAt(0, Pointi(0, 0), ring));
    auto seeded = mm6::seedMM6CreationItems(party, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(seeded);

    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    auto known = mm6::captureFirstMapKnownSeed(
        party, npcs, timer, overlays, "First map", 5, GameTitle::MM6);
    known.chest.lcgState = 17;
    std::vector<std::string> names;
    for (int i = 0; i < 52; ++i)
        names.push_back("d" + std::to_string(i) + ".dlv");
    names.push_back("oute3.ddm");
    for (int i = 1; i < 15; ++i)
        names.push_back("o" + std::to_string(i) + ".ddm");
    const auto defaultReader = [](std::string_view name) {
        const std::vector<std::uint8_t> bytes(name.ends_with(".dlv")
            ? 8 + 875 + 3 * 4 + 200 * 80 + 200 + 56
            : 8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
        return Blob::copy(bytes.data(), bytes.size());
    };
    const auto baseReader = [](std::string_view name) {
        std::vector<std::uint8_t> bytes(name.ends_with(".blv")
            ? 136 + 10 * 4
            : 160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
        if (name.ends_with(".blv"))
            bytes[136 + 4 * 4] = 200;
        return Blob::copy(bytes.data(), bytes.size());
    };
    const auto shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    auto prepared = mm6::prepareOfflineRawNewGameAutosave(
        known, seeded->party, seeded->inventory, seeded->cursor, shops, {}, names,
        defaultReader, baseReader,
        [](std::int32_t id) { return id >= 1 && id <= 580; },
        *scope.table, GameTitle::MM6);
    EXPECT_EQ(prepared.staged.saved.inventory, seeded->inventory);
    EXPECT_EQ(prepared.staged.saved.inventory.characters[0].items[0].itemNumberRaw(), 10u);
    EXPECT_EQ(prepared.staged.saved.heldItem.present, false);
}
