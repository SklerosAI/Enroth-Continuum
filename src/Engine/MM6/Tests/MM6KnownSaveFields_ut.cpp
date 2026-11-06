#include <algorithm>
#include <array>
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
    fields.chest = {0x12345678, {}, "d0.dlv", {1}};
    fields.chest.generatedArtifactFlags[7] = 1;
    fields.npcs.activeCount = 399;
    fields.npcs.npcData[398].name = "Synthetic NPC";
    fields.npcs.groups[50] = 23;
    fields.resistances.characters[0].base[0] = -5;
    fields.resistances.characters[3].bonus[10] = 17;
    fields.heldItem.present = true;
    fields.heldItem.itemId = 1;
    fields.buffs.party[0].expireTicks = 100;
    fields.buffs.party[0].power = 12;
    fields.buffs.characters[3][23].isGM = true;
    fields.hirelings.displayNames[0] = "Synthetic hireling";
    fields.recovery.playingTicks = 42;
    fields.recovery.lastRegenerationTicks = 40;
    fields.recovery.characterRecoveryTicks[0] = 7;
    fields.recovery.eventClocks.counters[2] = 31;
    fields.recovery.eventClocks.history[17] = -1;
    fields.recovery.eventClocks.unknownTimes[19] = 9;
    return fields;
}

} // namespace

UNIT_TEST(MM6KnownSaveFields, AssemblesAllSeventeenRequiredSections) {
    mm6::KnownSaveFields fields = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(fields);
    mm6::SaveContainer container = mm6::decodeSaveContainer(bytes);
    ASSERT_EQ(container.sections.size(), 17);
    for (const mm6::SaveSection &section : container.sections)
        EXPECT_TRUE(section.required);
    EXPECT_EQ(mm6::encodeKnownSave(mm6::decodeKnownSave(bytes)), bytes);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).resistances, fields.resistances);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).heldItem, fields.heldItem);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).buffs, fields.buffs);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).hirelings.displayNames[0], "Synthetic hireling");
    EXPECT_EQ(mm6::decodeKnownSave(bytes).recovery, fields.recovery);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).recovery.eventClocks.counters[2], 31);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).chest, fields.chest);

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

UNIT_TEST(MM6KnownSaveFields, OldSixteenSectionCandidateRejectsBeforeMutation) {
    mm6::SaveContainer old = mm6::decodeSaveContainer(mm6::encodeKnownSave(exampleFields()));
    old.sections.erase(std::find_if(old.sections.begin(), old.sections.end(),
        [](const mm6::SaveSection &section) {
            return section.tag == std::array{'C', 'H', 'S', 'T'};
        }));
    ASSERT_EQ(old.sections.size(), 16);
    mm6::KnownSaveFields destination = exampleFields();
    destination.core.party.gold = 777;
    EXPECT_THROW(mm6::loadKnownSaveBeforeMutation(mm6::encodeSaveContainer(old), &destination),
                 std::exception);
    EXPECT_EQ(destination.core.party.gold, 777);
    EXPECT_EQ(destination.chest.lcgState, 0x12345678);
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
    fields.chest.activeDeltaName = "o0.ddm";
    fields.chest.rawActiveDelta = {2};
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
    fields.recovery.playingTicks++;
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

    container = mm6::decodeSaveContainer(mm6::encodeKnownSave(exampleFields()));
    changed = exampleFields();
    changed.recovery.playingTicks = 43;
    container.sections[container.sections.size() - 2] = mm6::encodeRecoverySection(changed.recovery);
    EXPECT_THROW((void) mm6::decodeKnownSave(mm6::encodeSaveContainer(container)), std::exception);
}

UNIT_TEST(MM6KnownSaveFields, LateDecodeFailureLeavesDestinationUntouched) {
    mm6::KnownSaveFields destination = exampleFields();
    destination.metadata.header.displayName = "Prior state";
    mm6::SaveContainer invalid = mm6::decodeSaveContainer(mm6::encodeKnownSave(exampleFields()));
    invalid.sections.back().payload[0] = 3; // Unsupported CHST version, with a valid outer CRC.
    EXPECT_THROW(mm6::loadKnownSaveBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination.metadata.header.displayName, "Prior state");
    EXPECT_EQ(destination.npcs.npcData[398].name, "Synthetic NPC");
    EXPECT_EQ(destination.resistances.characters[0].base[0], -5);
    EXPECT_EQ(destination.heldItem.itemId, 1);
    EXPECT_EQ(destination.buffs.party[0].power, 12);
    EXPECT_EQ(destination.hirelings.displayNames[0], "Synthetic hireling");
    EXPECT_EQ(destination.recovery.lastRegenerationTicks, 40);
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
