#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6KnownSaveFields.h"

namespace {

mm6::KnownSaveFields exampleFields() {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = 42;
    fields.core.party.activeCharacter = 2;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "Offline test";
    fields.metadata.header.mapFileName = "d0.blv";
    fields.metadata.header.playingTicks = 42;
    fields.metadata.timer.elapsedTicks = 17; // Independent simulation clock.
    fields.metadata.ui.activeCharacter = 2;
    for (int i = 0; i < 52; ++i)
        fields.maps.deltas.push_back({"d" + std::to_string(i) + ".dlv", {1}});
    for (int i = 0; i < 15; ++i)
        fields.maps.deltas.push_back({"o" + std::to_string(i) + ".ddm", {2}});
    fields.npcs.activeCount = 399;
    fields.npcs.npcData[398].name = "Synthetic NPC";
    fields.npcs.groups[50] = 23;
    fields.resistances.characters[0].base[0] = -5;
    fields.resistances.characters[3].bonus[10] = 17;
    return fields;
}

} // namespace

UNIT_TEST(MM6KnownSaveFields, AssemblesAllTwelveRequiredSections) {
    mm6::KnownSaveFields fields = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(fields);
    mm6::SaveContainer container = mm6::decodeSaveContainer(bytes);
    ASSERT_EQ(container.sections.size(), 12);
    for (const mm6::SaveSection &section : container.sections)
        EXPECT_TRUE(section.required);
    EXPECT_EQ(mm6::encodeKnownSave(mm6::decodeKnownSave(bytes)), bytes);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).resistances, fields.resistances);

    std::reverse(container.sections.begin(), container.sections.end());
    EXPECT_EQ(mm6::encodeKnownSave(mm6::decodeKnownSave(mm6::encodeSaveContainer(container))), bytes);
}

UNIT_TEST(MM6KnownSaveFields, EverySectionIsMandatoryAndExtrasReject) {
    mm6::SaveContainer baseline = mm6::decodeSaveContainer(mm6::encodeKnownSave(exampleFields()));
    for (size_t index = 0; index < baseline.sections.size(); ++index) {
        mm6::SaveContainer missing = baseline;
        missing.sections.erase(missing.sections.begin() + index);
        EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(missing)), std::exception);
        mm6::SaveContainer optional = baseline;
        optional.sections[index].required = false;
        EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(optional)), std::exception);
    }
    mm6::SaveContainer extra = baseline;
    extra.sections.push_back({{'M', 'E', 'T', 'A'}, false, {1}});
    EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(extra)), std::exception);
    extra.sections.back().tag = {'E', 'X', 'T', 'R'};
    EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(extra)), std::exception);
}

UNIT_TEST(MM6KnownSaveFields, RequiresSixtySevenDeltasAndActiveCompanion) {
    mm6::KnownSaveFields fields = exampleFields();
    fields.maps.deltas.pop_back();
    EXPECT_THROW((void) mm6::encodeKnownSave(fields), std::exception);
    fields = exampleFields();
    fields.maps.deltas[0].name = "other.dlv";
    EXPECT_THROW((void) mm6::encodeKnownSave(fields), std::exception);
    fields = exampleFields();
    fields.metadata.header.mapFileName = "o0.odm";
    EXPECT_NO_THROW((void) mm6::encodeKnownSave(fields));
    fields.maps.deltas.back().name = fields.maps.deltas[1].name;
    EXPECT_THROW((void) mm6::encodeKnownSave(fields), std::exception);
}

UNIT_TEST(MM6KnownSaveFields, DuplicatedClockAndSelectionMustAgree) {
    mm6::KnownSaveFields fields = exampleFields();
    fields.metadata.header.playingTicks++;
    EXPECT_THROW((void) mm6::encodeKnownSave(fields), std::exception);
    fields = exampleFields();
    fields.metadata.ui.activeCharacter = 1;
    EXPECT_THROW((void) mm6::encodeKnownSave(fields), std::exception);
    fields = exampleFields();
    fields.metadata.timer.elapsedTicks = 500;
    EXPECT_NO_THROW((void) mm6::encodeKnownSave(fields));
}

UNIT_TEST(MM6KnownSaveFields, ValidCrcCrossFieldMismatchRejectsDecode) {
    mm6::SaveContainer container = mm6::decodeSaveContainer(mm6::encodeKnownSave(exampleFields()));
    mm6::KnownSaveFields changed = exampleFields();
    changed.metadata.header.playingTicks = 43;
    container.sections[5] = mm6::encodeMetadataSections(changed.metadata)[0];
    EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(container)), std::exception);

    changed = exampleFields();
    changed.metadata.header.mapFileName = "absent.blv";
    container.sections[5] = mm6::encodeMetadataSections(changed.metadata)[0];
    EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(container)), std::exception);
}

UNIT_TEST(MM6KnownSaveFields, LateDecodeFailureLeavesDestinationUntouched) {
    mm6::KnownSaveFields destination = exampleFields();
    destination.metadata.header.displayName = "Prior state";
    mm6::SaveContainer invalid = mm6::decodeSaveContainer(mm6::encodeKnownSave(exampleFields()));
    invalid.sections.back().payload[0] = 2; // RSTS version, with a valid outer CRC.
    EXPECT_THROW(mm6::loadKnownSaveBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination.metadata.header.displayName, "Prior state");
    EXPECT_EQ(destination.npcs.npcData[398].name, "Synthetic NPC");
    EXPECT_EQ(destination.resistances.characters[0].base[0], -5);
    EXPECT_THROW(mm6::loadKnownSaveBeforeMutation(mm6::encodeKnownSave(exampleFields()), nullptr), std::exception);
}

UNIT_TEST(MM6KnownSaveFields, RejectsWrongTitleAndCorruptEnvelope) {
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(exampleFields());
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeKnownSave(bytes), std::exception);
    bytes = mm6::encodeKnownSave(exampleFields());
    bytes.back() ^= 1;
    EXPECT_THROW((void) mm6::decodeKnownSave(bytes), std::exception);
}
