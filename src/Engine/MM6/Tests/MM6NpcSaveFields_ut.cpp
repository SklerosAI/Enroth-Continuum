#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6NpcSaveFields.h"

namespace {

mm6::NpcSaveFields exampleFields() {
    mm6::NpcSaveFields fields;
    fields.activeCount = 71;
    NPCData &npc = fields.npcData[70];
    npc.name = "Modded companion";
    npc.portraitId = 999;
    npc.flags = NpcFlags(0x80000083u); // Preserve unknown flag bits.
    npc.fame = -123;
    npc.rep = 456;
    npc.house = HOUSE_MM6_LAST;
    npc.profession = NPC_PROFESSION_MM6_LAST;
    npc.greetingIndex = -2;
    npc.canJoin = true;
    npc.field_24 = -3;
    npc.dialogue_1_evt_id = 101;
    npc.dialogue_2_evt_id = 102;
    npc.dialogue_3_evt_id = 103;
    npc.dialogue_4_evt_id = 104;
    npc.dialogue_5_evt_id = 105;
    npc.dialogue_6_evt_id = 106;
    npc.sex = SEX_FEMALE;
    npc.hasUsedAbility = -4;
    npc.newsTopic = 99;
    fields.npcData[69].house = static_cast<HouseId>(-1); // Source table's no-house sentinel.
    fields.groups[50] = 65535;
    return fields;
}

std::vector<std::uint8_t> saveBytes(const mm6::NpcSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeNpcSection(fields)}});
}

} // namespace

UNIT_TEST(MM6NpcSaveFields, RoundTripsAllRuntimeFieldsAndOpaqueBits) {
    mm6::NpcSaveFields fields = exampleFields();
    std::vector<std::uint8_t> bytes = saveBytes(fields);
    mm6::NpcSaveFields decoded = mm6::decodeNpcSection(mm6::decodeSaveContainer(bytes));
    EXPECT_EQ(mm6::encodeNpcSection(decoded), mm6::encodeNpcSection(fields));
    EXPECT_EQ(std::to_underlying(decoded.npcData[69].house), -1);

    mm6::NpcSaveFields destination;
    mm6::loadNpcFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(mm6::encodeNpcSection(destination), mm6::encodeNpcSection(fields));
}

UNIT_TEST(MM6NpcSaveFields, StagesOnCopyAndRetainsOriginalTables) {
    NPCStats live{};
    live.pNPCData = exampleFields().npcData;
    live.pGroups[50] = 6;
    live.uNumNewNPCs = 71;
    live.pOriginalNPCData[1].name = "Original table";
    live.pOriginalGroups[2] = 7;
    live.field_17FC8 = 13;

    mm6::NpcSaveFields captured = mm6::captureNpcFields(live, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeNpcSection(captured), mm6::encodeNpcSection(mm6::decodeNpcSection(
        mm6::decodeSaveContainer(saveBytes(captured)))));

    mm6::NpcSaveFields replacement = exampleFields();
    replacement.npcData[70].name = "Changed companion";
    replacement.groups[50] = 42;
    NPCStats staged = mm6::stageNpcStats(live, saveBytes(replacement), GameTitle::MM6);
    EXPECT_EQ(staged.pNPCData[70].name, "Changed companion");
    EXPECT_EQ(live.pNPCData[70].name, "Modded companion");
    EXPECT_EQ(staged.pGroups[50], 42);
    EXPECT_EQ(live.pGroups[50], 6);
    EXPECT_EQ(staged.uNumNewNPCs, 71);
    EXPECT_EQ(staged.pOriginalNPCData[1].name, "Original table");
    EXPECT_EQ(staged.pOriginalGroups[2], 7);
    EXPECT_EQ(staged.field_17FC8, 13);
}

UNIT_TEST(MM6NpcSaveFields, RejectsInvalidCountsVersionAndTrailingData) {
    mm6::SaveSection section = mm6::encodeNpcSection(exampleFields());
    section.payload[1] = 0;
    EXPECT_THROW((void) mm6::decodeNpcSection({{section}}), std::exception);

    section = mm6::encodeNpcSection(exampleFields());
    section.payload[4] = 0xff;
    section.payload[5] = 0x01; // Active count 511 exceeds 501 slots.
    EXPECT_THROW((void) mm6::decodeNpcSection({{section}}), std::exception);

    section = mm6::encodeNpcSection(exampleFields());
    section.payload[0] = 2;
    EXPECT_THROW((void) mm6::decodeNpcSection({{section}}), std::exception);

    section = mm6::encodeNpcSection(exampleFields());
    section.payload.push_back(0);
    EXPECT_THROW((void) mm6::decodeNpcSection({{section}}), std::exception);

    mm6::NpcSaveFields invalid = exampleFields();
    invalid.activeCount = 502;
    EXPECT_THROW((void) mm6::encodeNpcSection(invalid), std::exception);
    invalid = exampleFields();
    invalid.npcData[70].name = std::string(1025, 'X');
    EXPECT_THROW((void) mm6::encodeNpcSection(invalid), std::exception);
    invalid = exampleFields();
    invalid.npcData[70].house = static_cast<HouseId>(-2);
    EXPECT_THROW((void) mm6::encodeNpcSection(invalid), std::exception);
    invalid.npcData[70].house = static_cast<HouseId>(558);
    EXPECT_THROW((void) mm6::encodeNpcSection(invalid), std::exception);
}

UNIT_TEST(MM6NpcSaveFields, RejectsMalformedRecordAndLeavesLiveStateUntouched) {
    NPCStats live{};
    live.pNPCData[70].name = "Unchanged";
    mm6::SaveSection section = mm6::encodeNpcSection(exampleFields());
    section.payload[36] = 2; // Slot zero has an invalid canJoin byte.
    std::vector<std::uint8_t> invalid = mm6::encodeSaveContainer({{section}});
    EXPECT_THROW((void) mm6::stageNpcStats(live, invalid, GameTitle::MM6), std::exception);
    EXPECT_EQ(live.pNPCData[70].name, "Unchanged");

    section = mm6::encodeNpcSection(exampleFields());
    section.payload.pop_back();
    EXPECT_THROW((void) mm6::decodeNpcSection({{section}}), std::exception);

    section = mm6::encodeNpcSection(exampleFields());
    section.required = false;
    EXPECT_THROW((void) mm6::decodeNpcSection({{section}}), std::exception);
    EXPECT_THROW((void) mm6::decodeNpcSection({}), std::exception);
}

UNIT_TEST(MM6NpcSaveFields, RejectsForeignTitleAndCorruptContainer) {
    NPCStats live{};
    std::vector<std::uint8_t> bytes = saveBytes(exampleFields());
    EXPECT_THROW((void) mm6::captureNpcFields(live, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageNpcStats(live, bytes, GameTitle::MM7), std::exception);
    bytes.back() ^= 0x80;
    mm6::NpcSaveFields destination = exampleFields();
    EXPECT_THROW(mm6::loadNpcFieldsBeforeMutation(bytes, &destination), std::exception);
    EXPECT_EQ(destination.npcData[70].name, "Modded companion");
}
