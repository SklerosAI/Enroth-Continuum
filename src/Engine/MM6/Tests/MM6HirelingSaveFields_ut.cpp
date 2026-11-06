#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6HirelingSaveFields.h"
#include "Engine/MM6/MM6NpcSaveFields.h"

namespace {

mm6::HirelingSaveFields exampleFields() {
    mm6::HirelingSaveFields fields;
    NPCData &first = fields.hirelings[0];
    first.name = "Companion";
    first.portraitId = 653;
    first.flags = NpcFlags(0x80000080u);
    first.house = HOUSE_MM6_LAST;
    first.profession = NPC_PROFESSION_MM6_LAST;
    first.canJoin = true;
    first.dialogue_1_evt_id = 71;
    first.dialogue_2_evt_id = 72;
    first.hasUsedAbility = 1;
    fields.displayNames[0] = "Companion display";
    fields.sacrifice[0] = {true, 7, 100};
    fields.hirelings[1].name = "Second companion";
    fields.displayNames[1] = "Second display";
    return fields;
}

mm6::SaveContainer containerFor(const mm6::HirelingSaveFields &fields) {
    return {{mm6::encodeHirelingSection(fields)}};
}

} // namespace

UNIT_TEST(MM6HirelingSaveFields, FullNpcCopiesNamesAndSacrificeRoundTrip) {
    mm6::HirelingSaveFields fields = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(fields));
    mm6::HirelingSaveFields decoded = mm6::decodeHirelingSection(mm6::decodeSaveContainer(bytes));
    EXPECT_EQ(mm6::encodeHirelingSection(decoded), mm6::encodeHirelingSection(fields));
    EXPECT_EQ(decoded.hirelings[0].hasUsedAbility, 1);
    EXPECT_EQ(decoded.sacrifice[0], (mm6::SavedSacrificeStatus{true, 7, 100}));

    mm6::HirelingSaveFields destination;
    mm6::loadHirelingFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(mm6::encodeHirelingSection(destination), mm6::encodeHirelingSection(fields));
}

UNIT_TEST(MM6HirelingSaveFields, SharedNpcRecordCodecMatchesHirelingRecord) {
    NPCData source = exampleFields().hirelings[0];
    std::vector<std::uint8_t> encoded = mm6::encodeNpcRecord(source);
    NPCData decoded = mm6::decodeNpcRecord(encoded);
    EXPECT_EQ(mm6::encodeNpcRecord(decoded), encoded);
    encoded.push_back(0);
    EXPECT_THROW((void) mm6::decodeNpcRecord(encoded), std::exception);
}

UNIT_TEST(MM6HirelingSaveFields, RejectsInvalidNameNpcAndSacrifice) {
    mm6::HirelingSaveFields fields = exampleFields();
    fields.displayNames[0] = std::string(1025, 'A');
    EXPECT_THROW((void) mm6::encodeHirelingSection(fields), std::exception);
    fields = exampleFields();
    fields.hirelings[0].profession = static_cast<NpcProfession>(78);
    EXPECT_THROW((void) mm6::encodeHirelingSection(fields), std::exception);
    fields = exampleFields();
    fields.sacrifice[0].endTicks = 0;
    EXPECT_THROW((void) mm6::encodeHirelingSection(fields), std::exception);
    fields = exampleFields();
    fields.sacrifice[0].elapsedTicks = -1;
    EXPECT_THROW((void) mm6::encodeHirelingSection(fields), std::exception);
}

UNIT_TEST(MM6HirelingSaveFields, RejectsMalformedRequiredSection) {
    mm6::SaveContainer invalid = containerFor(exampleFields());
    invalid.sections[0].payload[0] = 2;
    EXPECT_THROW((void) mm6::decodeHirelingSection(invalid), std::exception);
    invalid = containerFor(exampleFields());
    invalid.sections[0].payload[1] = 3;
    EXPECT_THROW((void) mm6::decodeHirelingSection(invalid), std::exception);
    invalid = containerFor(exampleFields());
    invalid.sections[0].payload.pop_back();
    EXPECT_THROW((void) mm6::decodeHirelingSection(invalid), std::exception);
    invalid = containerFor(exampleFields());
    invalid.sections[0].required = false;
    EXPECT_THROW((void) mm6::decodeHirelingSection(invalid), std::exception);
    EXPECT_THROW((void) mm6::decodeHirelingSection({}), std::exception);
}

UNIT_TEST(MM6HirelingSaveFields, LateValidCrcErrorLeavesDestinationUntouched) {
    mm6::HirelingSaveFields destination = exampleFields();
    mm6::SaveContainer invalid = containerFor(exampleFields());
    invalid.sections[0].payload.back() = 0xff; // Negative second end timer.
    EXPECT_THROW(mm6::loadHirelingFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination),
                 std::exception);
    EXPECT_EQ(destination.hirelings[0].name, "Companion");
    EXPECT_EQ(destination.sacrifice[0], (mm6::SavedSacrificeStatus{true, 7, 100}));
    EXPECT_THROW(mm6::loadHirelingFieldsBeforeMutation(
                     mm6::encodeSaveContainer(containerFor(exampleFields())), nullptr), std::exception);
}

UNIT_TEST(MM6HirelingSaveFields, RejectsWrongTitleAndCorruptContainer) {
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(exampleFields()));
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);
    bytes = mm6::encodeSaveContainer(containerFor(exampleFields()));
    bytes.back() ^= 1;
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);
}

UNIT_TEST(MM6HirelingSaveFields, CorrespondenceDoesNotRequireQuestFollowerInPartySlots) {
    mm6::HirelingSaveFields hirelings;
    mm6::NpcSaveFields npcs;
    npcs.activeCount = 4;
    npcs.npcData[3].name = "Quest follower";
    npcs.npcData[3].flags = NPC_HIRED;
    auto result = mm6::inspectHirelingCorrespondence(hirelings, npcs);
    EXPECT_EQ(result[0], mm6::HirelingCorrespondence());
    EXPECT_EQ(result[1], mm6::HirelingCorrespondence());
}

UNIT_TEST(MM6HirelingSaveFields, CorrespondenceReportsButDoesNotEnforceTableFlag) {
    mm6::HirelingSaveFields hirelings = exampleFields();
    mm6::NpcSaveFields npcs;
    npcs.activeCount = 11;
    npcs.npcData[10] = hirelings.hirelings[0];
    auto result = mm6::inspectHirelingCorrespondence(hirelings, npcs);
    EXPECT_EQ(result[0], (mm6::HirelingCorrespondence{true, 1, 1}));
    EXPECT_EQ(result[1], (mm6::HirelingCorrespondence{true, 0, 0})); // Generated or renamed copy.

    npcs.npcData[10].flags &= ~NPC_HIRED; // An event can clear the table flag only.
    result = mm6::inspectHirelingCorrespondence(hirelings, npcs);
    EXPECT_EQ(result[0], (mm6::HirelingCorrespondence{true, 1, 0}));
}

UNIT_TEST(MM6HirelingSaveFields, CorrespondenceExposesAmbiguityAndRejectsInvalidInputs) {
    mm6::HirelingSaveFields hirelings = exampleFields();
    mm6::NpcSaveFields npcs;
    npcs.activeCount = 4;
    for (int index = 1; index < 4; ++index)
        npcs.npcData[index] = hirelings.hirelings[0];
    npcs.npcData[2].flags &= ~NPC_HIRED;
    auto result = mm6::inspectHirelingCorrespondence(hirelings, npcs);
    EXPECT_EQ(result[0], (mm6::HirelingCorrespondence{true, 3, 2}));

    npcs.activeCount = 502;
    EXPECT_THROW((void) mm6::inspectHirelingCorrespondence(hirelings, npcs), std::exception);
    npcs.activeCount = 4;
    hirelings.hirelings[0].profession = static_cast<NpcProfession>(78);
    EXPECT_THROW((void) mm6::inspectHirelingCorrespondence(hirelings, npcs), std::exception);
}
