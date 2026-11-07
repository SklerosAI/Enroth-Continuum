#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6HeldItemSaveFields.h"
#include "Engine/MM6/MM6RawHeldItemSaveFields.h"

namespace {

mm6::RawHeldItemSaveFields example() {
    mm6::ItemRecord28 item;
    item.setWord(0, 501);
    item.setWord(4, 0x81234567);
    item.setWord(8, 0xfedcba98);
    item.setWord(12, 0x12345678);
    item.setWord(16, 0x87654321);
    item.setWord(20, 0x76543210);
    item[24] = 0xa1;
    item[25] = 0xb2;
    item[26] = 0xc3;
    item[27] = 0xd4;
    return {true, item};
}

std::vector<std::uint8_t> saveBytes(const mm6::RawHeldItemSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeRawHeldItemSection(fields, GameTitle::MM6)}});
}

} // namespace

UNIT_TEST(MM6RawHeldItemSaveFields, EmptyAndPresentRoundTripPreservesAllBytes) {
    mm6::RawHeldItemSaveFields empty;
    empty.record[25] = 0xa5;
    empty.record[26] = 0xb6;
    empty.record[27] = 0xc7;
    auto emptySection = mm6::encodeRawHeldItemSection(empty, GameTitle::MM6);
    EXPECT_EQ(emptySection.payload.size(), 32);
    EXPECT_EQ(emptySection.payload[1], 0);
    EXPECT_EQ(emptySection.payload[29], 0xa5);
    EXPECT_EQ(emptySection.payload[30], 0xb6);
    EXPECT_EQ(emptySection.payload[31], 0xc7);
    EXPECT_EQ(mm6::decodeRawHeldItemSection(mm6::decodeSaveContainer(saveBytes(empty)),
                                             GameTitle::MM6), empty);

    mm6::RawHeldItemSaveFields held = example();
    auto section = mm6::encodeRawHeldItemSection(held, GameTitle::MM6);
    EXPECT_EQ(section.payload.size(), 32);
    EXPECT_EQ(section.payload[0], 2);
    EXPECT_EQ(section.payload[1], 1);
    for (int i = 0; i < 28; ++i)
        EXPECT_EQ(section.payload[4 + i], held.record[i]);
    EXPECT_EQ(mm6::decodeRawHeldItemSection(mm6::decodeSaveContainer(saveBytes(held)),
                                             GameTitle::MM6), held);

    mm6::RawHeldItemSaveFields destination;
    mm6::loadRawHeldItemBeforeMutation(saveBytes(held), &destination, GameTitle::MM6);
    EXPECT_EQ(destination, held);
}

UNIT_TEST(MM6RawHeldItemSaveFields, VersionPresenceAndMalformedPayloadRejectBeforeMutation) {
    const mm6::RawHeldItemSaveFields before = example();
    mm6::RawHeldItemSaveFields destination = before;
    const auto valid = mm6::encodeRawHeldItemSection(before, GameTitle::MM6);

    auto rejects = [&](mm6::SaveSection section) {
        EXPECT_THROW(mm6::loadRawHeldItemBeforeMutation(
                         mm6::encodeSaveContainer({{section}}), &destination, GameTitle::MM6),
                     std::exception);
        EXPECT_EQ(destination, before);
    };
    auto section = valid;
    section.payload[0] = 1;
    rejects(section);
    section = valid;
    section.payload[1] = 2;
    rejects(section);
    section = valid;
    section.payload[2] = 1;
    rejects(section);
    section = valid;
    section.payload[3] = 1;
    rejects(section);
    section = valid;
    section.payload.pop_back();
    rejects(section);
    section = valid;
    section.required = false;
    rejects(section);
    section = valid;
    section.payload[4] = 0; // Zero source ID, but a valid container checksum.
    section.payload[5] = 0;
    rejects(section);
    section = mm6::encodeRawHeldItemSection({}, GameTitle::MM6);
    section.payload[4] = 1; // An absent cursor must have a cleared 25-byte prefix.
    rejects(section);
    section = mm6::encodeRawHeldItemSection({}, GameTitle::MM6);
    section.payload.pop_back();
    rejects(section);

    mm6::SaveContainer duplicate{{valid, valid}};
    EXPECT_THROW((void) mm6::decodeRawHeldItemSection(duplicate, GameTitle::MM6), std::exception);
    EXPECT_EQ(destination, before);
    auto damagedEnvelope = saveBytes(before);
    damagedEnvelope.back() ^= 0x01;
    EXPECT_THROW(mm6::loadRawHeldItemBeforeMutation(damagedEnvelope, &destination, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(destination, before);
}

UNIT_TEST(MM6RawHeldItemSaveFields, RejectsForeignTitleAndOldWireVersion) {
    const mm6::RawHeldItemSaveFields before = example();
    mm6::RawHeldItemSaveFields destination = before;
    EXPECT_THROW((void) mm6::encodeRawHeldItemSection(before, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::decodeRawHeldItemSection(
                     mm6::decodeSaveContainer(saveBytes(before)), GameTitle::MM7), std::exception);
    EXPECT_THROW(mm6::loadRawHeldItemBeforeMutation(saveBytes(before), &destination, GameTitle::MM7),
                 std::exception);
    EXPECT_EQ(destination, before);
    EXPECT_THROW(mm6::loadRawHeldItemBeforeMutation(saveBytes(before), nullptr, GameTitle::MM6),
                 std::exception);

    const mm6::SaveContainer v1{{mm6::encodeHeldItemSection({})}};
    EXPECT_THROW((void) mm6::decodeRawHeldItemSection(v1, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::loadRawHeldItemBeforeMutation(mm6::encodeSaveContainer(v1), &destination,
                                                    GameTitle::MM6), std::exception);
    EXPECT_EQ(destination, before);
    const mm6::SaveContainer v2{{mm6::encodeRawHeldItemSection(before, GameTitle::MM6)}};
    EXPECT_THROW((void) mm6::decodeHeldItemSection(v2), std::exception);

    auto missing = before;
    missing.record.setWord(0, 581);
    EXPECT_THROW((void) mm6::encodeRawHeldItemSection(missing, GameTitle::MM6), std::exception);
    auto uncleared = mm6::RawHeldItemSaveFields{};
    uncleared.record[24] = 1;
    EXPECT_THROW((void) mm6::encodeRawHeldItemSection(uncleared, GameTitle::MM6), std::exception);
}
