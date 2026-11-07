#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawPartyInventoryStage.h"

namespace {

int sourceId(int type) { return type == 15 ? 221 : 100 + type; }

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    TableScope() {
        pItemTable = table.get();
        for (int type = 0; type <= 20; ++type) {
            const ItemId id = static_cast<ItemId>(sourceId(type));
            table->items[id].iconName = "source-kind";
            table->items[id].type = static_cast<ItemType>(type);
            table->itemSizes[id] = Sizei(1, 1);
        }
    }

    ~TableScope() { pItemTable = previous; }
};

mm6::RawInventoryFields allPlainSourceKinds() {
    mm6::RawInventoryFields raw;
    raw.ready = true;
    for (std::size_t character = 0; character < raw.characters.size(); ++character) {
        for (int type = 0; type <= 20; ++type) {
            const int slot = 117 + type;
            const int id = sourceId(type);
            auto &record = raw.characters[character].items[slot];
            record.setWord(0, id);
            record[26] = static_cast<std::uint8_t>(0x30 + character);
            record[27] = static_cast<std::uint8_t>(0x60 + type);
            raw.characters[character].gridWords[type] = slot + 1;
        }
        raw.characters[character].items[75][27] = 0xee; // Unreferenced native tail.
    }
    for (auto &character : raw.characters) {
        character.items[129].setWord(16, 4); // Source-typed wand.
        character.items[129][25] = 6;
        character.items[135].setWord(12, 80); // Source-typed gold.
    }
    return raw;
}

} // namespace

UNIT_TEST(MM6RawPartyInventoryStage, StagesAllPlainSourceTypesOnFreshParty) {
    TableScope scope;
    Party baseline;
    baseline.uNumGold = 41;
    baseline.pCharacters[0].inventory = CharacterInventory::forMM6();
    const auto raw = allPlainSourceKinds();
    auto staged = mm6::stageRawPartyInventories(baseline, raw, *scope.table, GameTitle::MM6);
    EXPECT_EQ(staged.party.uNumGold, 41);
    for (std::size_t character = 0; character < 4; ++character) {
        EXPECT_EQ(staged.party.pCharacters[character].inventory.capacity(), 138);
        for (int type = 0; type <= 20; ++type) {
            const auto entry = staged.party.pCharacters[character].inventory.entry(117 + type);
            ASSERT_TRUE(entry);
            EXPECT_EQ(entry->type(), static_cast<ItemType>(type));
            EXPECT_EQ(entry->mm6RawRecord, raw.characters[character].items[117 + type]);
        }
    }
    EXPECT_EQ(mm6::captureRawPartyInventories(staged, *scope.table, GameTitle::MM6), raw);
    EXPECT_FALSE(baseline.pCharacters[0].inventory.entry(137));

    staged.party.pCharacters[2].inventory.entry(135)->goldAmount = 91;
    staged.party.pCharacters[3].inventory.entry(129)->numCharges = 3;
    auto changed = mm6::captureRawPartyInventories(staged, *scope.table, GameTitle::MM6);
    EXPECT_EQ(changed.characters[2].items[135].secondBonusRaw(), 91);
    EXPECT_EQ(changed.characters[3].items[129].chargesRaw(), 3);
    EXPECT_EQ(changed.characters[0].items[75][27], 0xee);
    EXPECT_EQ(changed.characters[2].items[135][26], 0x32);
    EXPECT_EQ(changed.characters[3].items[129][27], 0x6c);
}

UNIT_TEST(MM6RawPartyInventoryStage, StandardEquipmentAndUnsupportedUnionFailClosed) {
    TableScope scope;
    Party baseline;
    baseline.uNumGold = 17;
    auto raw = allPlainSourceKinds();
    raw.characters[0].items[120].setWord(4, 12);
    raw.characters[0].items[120].setWord(8, 7);
    auto staged = mm6::stageRawPartyInventories(baseline, raw, *scope.table, GameTitle::MM6);
    ASSERT_TRUE(staged.party.pCharacters[0].inventory.entry(120)->standardEnchantment);
    staged.party.pCharacters[0].inventory.entry(120)->standardEnchantmentStrength = 9;
    auto changed = mm6::captureRawPartyInventories(staged, *scope.table, GameTitle::MM6);
    EXPECT_EQ(changed.characters[0].items[120].bonusStrengthRaw(), 9);
    EXPECT_EQ(changed.characters[0].items[120][27], raw.characters[0].items[120][27]);

    const auto before = raw;
    raw.characters[3].items[131].setWord(4, 12); // Potion-power union is not mapped.
    EXPECT_THROW((void) mm6::stageRawPartyInventories(baseline, raw, *scope.table,
                                                      GameTitle::MM6), std::exception);
    EXPECT_EQ(baseline.uNumGold, 17);
    EXPECT_FALSE(baseline.pCharacters[3].inventory.entry(131));
    raw = before;
    raw.characters[3].items[117].setWord(12, 4); // Special enchantment is not mapped.
    EXPECT_THROW((void) mm6::stageRawPartyInventories(baseline, raw, *scope.table,
                                                      GameTitle::MM6), std::exception);
    raw = before;
    scope.table->items[static_cast<ItemId>(120)].rarity = RARITY_ARTIFACT;
    EXPECT_THROW((void) mm6::stageRawPartyInventories(baseline, raw, *scope.table,
                                                      GameTitle::MM6), std::exception);
    EXPECT_EQ(raw, before);
}

UNIT_TEST(MM6RawPartyInventoryStage, RejectsForeignTitleAndUnsupportedEditsBeforeCapture) {
    TableScope scope;
    Party baseline;
    const auto raw = allPlainSourceKinds();
    EXPECT_THROW((void) mm6::stageRawPartyInventories(baseline, raw, *scope.table,
                                                      GameTitle::MM7), std::exception);
    ItemTable foreignTable;
    EXPECT_THROW((void) mm6::stageRawPartyInventories(baseline, raw, foreignTable,
                                                      GameTitle::MM6), std::exception);
    auto staged = mm6::stageRawPartyInventories(baseline, raw, *scope.table, GameTitle::MM6);
    staged.party.pCharacters[3].inventory.entry(137)->mm6RawRecord.reset();
    EXPECT_THROW((void) mm6::captureRawPartyInventories(staged, *scope.table,
                                                        GameTitle::MM6), std::exception);
    staged.party.pCharacters[3].inventory.entry(137)->mm6RawRecord = raw.characters[3].items[137];
    staged.party.pCharacters[3].inventory.take(staged.party.pCharacters[3].inventory.entry(137));
    EXPECT_THROW((void) mm6::captureRawPartyInventories(staged, *scope.table,
                                                        GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::captureRawPartyInventories(staged, *scope.table,
                                                        GameTitle::MM7), std::exception);
    EXPECT_EQ(raw, allPlainSourceKinds());
}
