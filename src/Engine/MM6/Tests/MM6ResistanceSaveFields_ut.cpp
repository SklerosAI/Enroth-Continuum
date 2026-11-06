#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ResistanceSaveFields.h"

namespace {

mm6::ResistanceSaveFields exampleFields() {
    mm6::ResistanceSaveFields fields;
    fields.characters[0].base[0] = -32768;
    fields.characters[1].bonus[6] = 32767;
    fields.characters[3].base[10] = -147;
    fields.characters[3].bonus[10] = 253;
    return fields;
}

std::vector<std::uint8_t> saveBytes(const mm6::ResistanceSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeResistanceSection(fields)}});
}

} // namespace

UNIT_TEST(MM6ResistanceSaveFields, ExactSignedRoundTripAndStagedLoad) {
    mm6::ResistanceSaveFields source = exampleFields();
    std::vector<std::uint8_t> bytes = saveBytes(source);
    EXPECT_EQ(mm6::decodeResistanceSection(mm6::decodeSaveContainer(bytes)), source);

    mm6::ResistanceSaveFields destination;
    mm6::loadResistanceFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, source);
}

UNIT_TEST(MM6ResistanceSaveFields, RejectsMalformedBeforeMutation) {
    const mm6::ResistanceSaveFields before = exampleFields();
    mm6::ResistanceSaveFields destination = before;
    mm6::SaveContainer container{{mm6::encodeResistanceSection(before)}};

    container.sections[0].payload[2] = 12;
    EXPECT_THROW(mm6::loadResistanceFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeResistanceSection(before)}};
    container.sections[0].payload.pop_back();
    EXPECT_THROW(mm6::loadResistanceFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeResistanceSection(before)}};
    container.sections[0].required = false;
    EXPECT_THROW(mm6::loadResistanceFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    std::vector<std::uint8_t> wrongTitle = saveBytes(before);
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::loadResistanceFieldsBeforeMutation(wrongTitle, &destination), std::exception);
    EXPECT_EQ(destination, before);
}
