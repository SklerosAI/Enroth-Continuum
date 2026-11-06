#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6ItemIconGuard.h"
#include "Engine/Objects/Inventory.h"
#include "Engine/Tables/ItemTable.h"

namespace {

mm6::DurableSaveFields exampleFields() {
    mm6::DurableSaveFields fields;
    fields.bits.quest[0] = 0x81;
    fields.bits.quest[63] = 0x80;
    fields.bits.autonotes[25] = 0x40;
    fields.bits.awards[2][17] = 0x20;
    fields.bits.characterEvents[3][63] = 0x01;
    fields.effects.conditionTicks[1][16] = 1234567;

    mm6::DurableItem grid;
    grid.sourceIndex = 17;
    grid.zone = 1;
    grid.x = 4;
    grid.y = 3;
    grid.itemId = 501;
    grid.standardEnchantment = 6;
    grid.standardStrength = 13;
    grid.flags = 0x101;
    grid.expirationTicks = 9876;
    fields.inventory.characters[0].push_back(grid);

    mm6::DurableItem equipped;
    equipped.sourceIndex = 4;
    equipped.zone = 2;
    equipped.equipmentSlot = 10;
    equipped.itemId = 231;
    equipped.charges = 4;
    equipped.maxCharges = 7;
    fields.inventory.characters[0].push_back(equipped);
    return fields;
}

mm6::SaveContainer containerFor(const mm6::DurableSaveFields &fields) {
    auto sections = mm6::encodeDurableSections(fields);
    mm6::SaveContainer container;
    container.sections.assign(sections.begin(), sections.end());
    return container;
}

} // namespace

UNIT_TEST(MM6DurableSaveFields, BitConditionAndItemRoundTrip) {
    mm6::DurableSaveFields source = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(source));
    EXPECT_EQ(mm6::decodeDurableSections(mm6::decodeSaveContainer(bytes)), source);

    mm6::DurableSaveFields destination;
    mm6::loadDurableFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, source);
}

UNIT_TEST(MM6DurableSaveFields, RejectsInvalidFieldsBeforeDestinationMutation) {
    mm6::DurableSaveFields destination = exampleFields();
    const mm6::DurableSaveFields before = destination;
    mm6::SaveContainer invalid = containerFor(before);
    invalid.sections[1].payload[1] = 255; // Negative first condition timestamp, valid container CRC.
    invalid.sections[1].payload[2] = 255;
    invalid.sections[1].payload[3] = 255;
    invalid.sections[1].payload[4] = 255;
    invalid.sections[1].payload[5] = 255;
    invalid.sections[1].payload[6] = 255;
    invalid.sections[1].payload[7] = 255;
    invalid.sections[1].payload[8] = 255;
    EXPECT_THROW(mm6::loadDurableFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[2].payload[2] = 127; // Count above character capacity.
    EXPECT_THROW(mm6::loadDurableFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);
}

UNIT_TEST(MM6DurableSaveFields, RejectsMissingOptionalDuplicateAndForeign) {
    mm6::DurableSaveFields fields = exampleFields();
    mm6::SaveContainer container = containerFor(fields);
    container.sections.pop_back();
    EXPECT_THROW((void) mm6::decodeDurableSections(container), std::exception);

    container = containerFor(fields);
    container.sections[0].required = false;
    EXPECT_THROW((void) mm6::decodeDurableSections(container), std::exception);

    fields.inventory.characters[0][1].sourceIndex = 17;
    EXPECT_THROW((void) mm6::encodeDurableSections(fields), std::exception);

    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(exampleFields()));
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);
}

UNIT_TEST(MM6DurableSaveFields, SparseIndexedItemPlacementRejectsOverlap) {
    ItemTable localTable;
    ItemTable *previous = pItemTable;
    struct RestoreTable {
        ItemTable *previous;
        ~RestoreTable() { pItemTable = previous; }
    } restore{previous};
    pItemTable = &localTable;

    localTable.itemSizes[ITEM_CRUDE_LONGSWORD] = Sizei(2, 1);
    localTable.itemSizes[ITEM_BRASS_RING] = Sizei(1, 1);
    CharacterInventory inventory;
    Item sword;
    sword.itemId = ITEM_CRUDE_LONGSWORD;
    InventoryEntry restored = inventory.tryRestoreAt(17, Pointi(3, 2), sword);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored.index(), 17);
    EXPECT_EQ(inventory.entry(Pointi(4, 2)).index(), 17);
    EXPECT_FALSE(inventory.tryRestoreAt(4, Pointi(4, 2), sword));
    EXPECT_FALSE(inventory.tryRestoreAt(126, Pointi(0, 0), sword));

    Item ring;
    ring.itemId = ITEM_BRASS_RING;
    InventoryEntry equipped = inventory.tryRestoreAt(4, ITEM_SLOT_RING1, ring);
    ASSERT_TRUE(equipped);
    EXPECT_EQ(equipped.index(), 4);
    EXPECT_EQ(inventory.entry(ITEM_SLOT_RING1).index(), 4);
    EXPECT_FALSE(inventory.tryRestoreAt(3, ITEM_SLOT_RING1, ring));
    EXPECT_FALSE(inventory.tryRestoreAt(4, ITEM_SLOT_RING2, ring));
}

UNIT_TEST(MM6ItemIconGuard, NamedTemplatesRequireImagesButPlaceholdersDoNot) {
    ItemTable table;
    table.items[ITEM_CRUDE_LONGSWORD].iconName = "present";
    table.items[ITEM_BRASS_RING].iconName = "missing";

    auto exists = [](std::string_view name) { return name == "present"; };
    EXPECT_EQ(mm6::collectMissingItemIconIds(table, exists),
              (std::vector<ItemId>{ITEM_BRASS_RING}));

    table.items[ITEM_BRASS_RING].iconName.clear();
    EXPECT_TRUE(mm6::collectMissingItemIconIds(table, exists).empty());
    table.items[ITEM_BRASS_RING].iconName = "present";
    EXPECT_TRUE(mm6::collectMissingItemIconIds(table, exists).empty());
}
