#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6HeldItemSaveFields.h"
#include "Engine/Tables/ItemTable.h"
#include "GUI/GUIMessageQueue.h"

namespace {

mm6::HeldItemSaveFields example() {
    mm6::HeldItemSaveFields fields;
    fields.present = true;
    fields.itemId = 501;
    fields.potionPower = -19;
    fields.goldAmount = 345;
    fields.standardEnchantment = 6;
    fields.standardStrength = 11;
    fields.specialEnchantment = 37;
    fields.charges = 3;
    fields.maxCharges = 8;
    fields.flags = 0x80000101u;
    fields.expirationTicks = 123456789;
    return fields;
}

std::vector<std::uint8_t> saveBytes(const mm6::HeldItemSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeHeldItemSection(fields)}});
}

} // namespace

UNIT_TEST(MM6HeldItemSaveFields, EmptyAndNonemptyRoundTrip) {
    mm6::HeldItemSaveFields empty;
    std::vector<std::uint8_t> emptyBytes = saveBytes(empty);
    EXPECT_EQ(mm6::decodeHeldItemSection(mm6::decodeSaveContainer(emptyBytes)), empty);
    EXPECT_EQ(mm6::encodeHeldItemSection(empty).payload.size(), 4);

    mm6::HeldItemSaveFields held = example();
    std::vector<std::uint8_t> bytes = saveBytes(held);
    EXPECT_EQ(mm6::decodeHeldItemSection(mm6::decodeSaveContainer(bytes)), held);
    EXPECT_EQ(mm6::encodeHeldItemSection(held).payload.size(), 52);

    mm6::HeldItemSaveFields destination;
    mm6::loadHeldItemBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, held);
}

UNIT_TEST(MM6HeldItemSaveFields, RejectsInvalidBeforeDestinationMutation) {
    mm6::HeldItemSaveFields before = example();
    mm6::HeldItemSaveFields destination = before;

    mm6::HeldItemSaveFields invalid = before;
    invalid.itemId = 581;
    EXPECT_THROW((void) mm6::encodeHeldItemSection(invalid), std::exception);
    invalid = before;
    invalid.lichJarCharacterIndex = 1;
    EXPECT_THROW((void) mm6::encodeHeldItemSection(invalid), std::exception);
    invalid = before;
    invalid.present = false;
    EXPECT_THROW((void) mm6::encodeHeldItemSection(invalid), std::exception);

    mm6::SaveContainer container{{mm6::encodeHeldItemSection(before)}};
    container.sections[0].payload[1] = 2;
    EXPECT_THROW(mm6::loadHeldItemBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeHeldItemSection(before)}};
    container.sections[0].payload.pop_back();
    EXPECT_THROW(mm6::loadHeldItemBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeHeldItemSection(before)}};
    container.sections[0].required = false;
    EXPECT_THROW(mm6::loadHeldItemBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    std::vector<std::uint8_t> wrongTitle = saveBytes(before);
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::loadHeldItemBeforeMutation(wrongTitle, &destination), std::exception);
    EXPECT_EQ(destination, before);
    EXPECT_THROW(mm6::loadHeldItemBeforeMutation(saveBytes(before), nullptr), std::exception);
}

UNIT_TEST(MM6HeldItemSaveFields, DetachedCursorIntentRequiresIdleActionsAndLoadedIcon) {
    ItemTable items;
    const ItemId id = static_cast<ItemId>(501);
    items.items[id].iconName = "held501";
    items.itemSizes[id] = Sizei(2, 3);
    mm6::PendingActionStateForSave idle;
    const auto empty = mm6::stageHeldCursorRestorePlan({}, items, idle, GameTitle::MM6);
    EXPECT_EQ(empty.iconName, "MICON1");
    EXPECT_FALSE(empty.itemCursor);
    EXPECT_TRUE(empty.resetClickOffset);

    mm6::HeldItemSaveFields held = example();
    const auto staged = mm6::stageHeldCursorRestorePlan(held, items, idle, GameTitle::MM6);
    EXPECT_EQ(staged.iconName, "held501");
    EXPECT_TRUE(staged.itemCursor);
    EXPECT_TRUE(staged.resetClickOffset);
    EXPECT_EQ(items.items[id].iconName, "held501");

    mm6::PendingActionStateForSave busy;
    busy.spellQueueActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.targetedSpellWindow = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.enchanting = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.turnModeActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.turnQueueActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.turnActionsPending = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.turnPendingFlag = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.timerTurnModeActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.restActionActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.restWindowActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.guiMessageQueued = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.movementActionQueued = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.delayedEnchantMessage = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.attackListActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    busy = {};
    busy.playerSelectionActive = true;
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, busy, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, idle, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::capturePendingActionStateForSave(GameTitle::MM7), std::exception);
    items.items[id].iconName.clear();
    EXPECT_THROW((void) mm6::stageHeldCursorRestorePlan(held, items, idle, GameTitle::MM6), std::exception);
    EXPECT_EQ(staged.iconName, "held501");
    EXPECT_NE(items.items[id].iconName, "held501");
}

UNIT_TEST(MM6HeldItemSaveFields, PendingUiQueryIncludesCurrentAndNextFrame) {
    GUIMessageQueue queue;
    EXPECT_FALSE(queue.hasPendingForSave());
    queue.addMessageNextFrame(UIMSG_Escape);
    EXPECT_FALSE(queue.haveMessages());
    EXPECT_TRUE(queue.hasPendingForSave());
    queue.clear(); // Current-frame-only clear must retain the queued next frame.
    EXPECT_TRUE(queue.hasPendingForSave());
    queue.swapFrames();
    EXPECT_TRUE(queue.haveMessages());
    EXPECT_TRUE(queue.hasPendingForSave());
    queue.clearAll();
    EXPECT_FALSE(queue.hasPendingForSave());
    queue.addMessageCurrentFrame(UIMSG_Escape);
    EXPECT_TRUE(queue.hasPendingForSave());
    queue.clearAll();
    EXPECT_FALSE(queue.hasPendingForSave());
}
