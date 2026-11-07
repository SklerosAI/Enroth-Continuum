#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawHeldCursorOwner.h"

namespace {

mm6::ItemRecord28 item() {
    mm6::ItemRecord28 record;
    record.setWord(0, 17);
    record.setWord(4, 0x12345678);
    record.setWord(20, 0x87654321);
    record[24] = 0xa1;
    record[25] = 0xb2;
    record[26] = 0xc3;
    record[27] = 0xd4;
    return record;
}

} // namespace

UNIT_TEST(MM6RawHeldCursorOwner, FullSetAndPartialClearRetainAbsentTail) {
    auto fresh = mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    ASSERT_TRUE(fresh);
    auto held = mm6::stageRawHeldCursorSet(*fresh, item(), GameTitle::MM6);
    ASSERT_TRUE(held);
    EXPECT_EQ(held->record, item());
    EXPECT_EQ(fresh->record.itemNumberRaw(), 0);
    const auto presentFields = mm6::captureRawHeldCursor(*held, GameTitle::MM6);
    ASSERT_TRUE(presentFields);
    EXPECT_TRUE(presentFields->present);
    EXPECT_EQ(mm6::stageRawHeldCursorFromFields(*presentFields, GameTitle::MM6), held);

    auto cleared = mm6::stageRawHeldCursorClear(*held, GameTitle::MM6);
    ASSERT_TRUE(cleared);
    for (int i = 0; i < 25; ++i)
        EXPECT_EQ(cleared->record[i], 0) << i;
    for (int i = 25; i < 28; ++i)
        EXPECT_EQ(cleared->record[i], held->record[i]) << i;
    const auto fields = mm6::captureRawHeldCursor(*cleared, GameTitle::MM6);
    ASSERT_TRUE(fields);
    EXPECT_FALSE(fields->present);
    EXPECT_EQ(fields->record, cleared->record);
    EXPECT_EQ(mm6::stageRawHeldCursorFromFields(*fields, GameTitle::MM6), cleared);
}

UNIT_TEST(MM6RawHeldCursorOwner, RejectsForeignTitleAndOccupiedOrInvalidOwner) {
    EXPECT_FALSE(mm6::freshRawHeldCursorOwner(GameTitle::MM7));
    auto fresh = mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    ASSERT_TRUE(fresh);
    EXPECT_FALSE(mm6::stageRawHeldCursorSet(*fresh, item(), GameTitle::MM7));
    EXPECT_FALSE(mm6::stageRawHeldCursorClear(*fresh, GameTitle::MM7));
    EXPECT_FALSE(mm6::captureRawHeldCursor(*fresh, GameTitle::MM7));
    auto held = mm6::stageRawHeldCursorSet(*fresh, item(), GameTitle::MM6);
    ASSERT_TRUE(held);
    EXPECT_FALSE(mm6::stageRawHeldCursorSet(*held, item(), GameTitle::MM6));

    mm6::RawHeldCursorOwner unready = *fresh;
    unready.ready = false;
    EXPECT_FALSE(mm6::stageRawHeldCursorClear(unready, GameTitle::MM6));
    mm6::RawHeldCursorOwner wrongTitle = *fresh;
    wrongTitle.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::stageRawHeldCursorSet(wrongTitle, item(), GameTitle::MM6));
    auto malformed = *fresh;
    malformed.record[20] = 1;
    EXPECT_FALSE(mm6::captureRawHeldCursor(malformed, GameTitle::MM6));
}

UNIT_TEST(MM6RawHeldCursorOwner, LateStagingFailureLeavesSourceUntouched) {
    auto fresh = mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    ASSERT_TRUE(fresh);
    const mm6::RawHeldCursorOwner before = *fresh;
    auto pending = mm6::stageRawHeldCursorSet(*fresh, item(), GameTitle::MM6);
    ASSERT_TRUE(pending);

    mm6::RawHeldItemSaveFields malformed{false, item()};
    EXPECT_FALSE(mm6::stageRawHeldCursorFromFields(malformed, GameTitle::MM6));
    EXPECT_EQ(*fresh, before); // Caller has not published the staged owner.
    EXPECT_EQ(pending->record, item());

    mm6::ItemRecord28 invalidIncoming = item();
    invalidIncoming.setWord(0, 581);
    EXPECT_FALSE(mm6::stageRawHeldCursorSet(*fresh, invalidIncoming, GameTitle::MM6));
    EXPECT_EQ(*fresh, before);
}
