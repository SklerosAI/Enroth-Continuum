#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BuffSaveFields.h"

namespace {

mm6::BuffSaveFields exampleFields() {
    mm6::BuffSaveFields fields;
    fields.party[7] = {123456, 12, 3, 50, -1, true}; // NPC flight may use isGM with Master.
    fields.characters[2][13] = {98765, 23, 2, 0, 2, false};
    fields.characters[3][23] = {-1, 0, 0, 0, -1, false};
    return fields;
}

mm6::SaveContainer containerFor(const mm6::BuffSaveFields &fields) {
    return {{mm6::encodeBuffSection(fields)}};
}

} // namespace

UNIT_TEST(MM6BuffSaveFields, PartyAndCharacterBuffsRoundTrip) {
    mm6::BuffSaveFields source = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(source));
    EXPECT_EQ(mm6::decodeBuffSection(mm6::decodeSaveContainer(bytes)), source);
    mm6::BuffSaveFields destination;
    mm6::loadBuffFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, source);
    EXPECT_TRUE(destination.party[7].isGM);
    EXPECT_EQ(destination.party[7].mastery, 3); // MM6 NPC convention, not GM skill.
}

UNIT_TEST(MM6BuffSaveFields, RejectsMalformedVersionCountAndLength) {
    mm6::SaveContainer invalid = containerFor(exampleFields());
    invalid.sections[0].payload[0] = 2;
    EXPECT_THROW((void) mm6::decodeBuffSection(invalid), std::exception);
    invalid = containerFor(exampleFields());
    invalid.sections[0].payload[1] = 21;
    EXPECT_THROW((void) mm6::decodeBuffSection(invalid), std::exception);
    invalid = containerFor(exampleFields());
    invalid.sections[0].payload.pop_back();
    EXPECT_THROW((void) mm6::decodeBuffSection(invalid), std::exception);
    invalid = containerFor(exampleFields());
    invalid.sections[0].required = false;
    EXPECT_THROW((void) mm6::decodeBuffSection(invalid), std::exception);
    EXPECT_THROW((void) mm6::decodeBuffSection({}), std::exception);
}

UNIT_TEST(MM6BuffSaveFields, RejectsInvalidTimeMasteryOverlayAndCaster) {
    mm6::BuffSaveFields fields = exampleFields();
    fields.party[0].expireTicks = -2;
    EXPECT_THROW((void) mm6::encodeBuffSection(fields), std::exception);
    fields = exampleFields();
    fields.party[0].mastery = 4; // MM6 has no Grandmaster skill.
    EXPECT_THROW((void) mm6::encodeBuffSection(fields), std::exception);
    fields = exampleFields();
    fields.party[0].overlayId = 51;
    EXPECT_THROW((void) mm6::encodeBuffSection(fields), std::exception);
    fields = exampleFields();
    fields.characters[0][0].caster = 4;
    EXPECT_THROW((void) mm6::encodeBuffSection(fields), std::exception);
}

UNIT_TEST(MM6BuffSaveFields, ValidCrcBadRecordLeavesDestinationUntouched) {
    mm6::BuffSaveFields destination = exampleFields();
    mm6::SaveContainer invalid = containerFor(exampleFields());
    invalid.sections[0].payload[14] = 4; // First party buff mastery.
    EXPECT_THROW(mm6::loadBuffFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, exampleFields());
    invalid = containerFor(exampleFields());
    invalid.sections[0].payload[18] = 2; // First party buff isGM must be boolean.
    EXPECT_THROW(mm6::loadBuffFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, exampleFields());
    EXPECT_THROW(mm6::loadBuffFieldsBeforeMutation(mm6::encodeSaveContainer(containerFor(exampleFields())), nullptr),
                 std::exception);
}

UNIT_TEST(MM6BuffSaveFields, RejectsWrongTitleAndCorruptContainer) {
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(exampleFields()));
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);
    bytes = mm6::encodeSaveContainer(containerFor(exampleFields()));
    bytes.back() ^= 1;
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);
}
