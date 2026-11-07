#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/Party.h"

namespace {

std::vector<std::uint8_t> resistanceBytes(const mm6::ResistanceSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeResistanceSection(fields)}});
}

std::vector<std::uint8_t> buffBytes(const mm6::BuffSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeBuffSection(fields)}});
}

} // namespace

UNIT_TEST(MM6DecodedEffectPartyStage, ResistanceFieldsMatchV1ByteStage) {
    Party baseline;
    baseline.uNumGold = 77;
    const auto before = mm6::captureResistanceFields(baseline, GameTitle::MM6);
    auto fields = before;
    fields.characters[0].base[0] = -32768;
    fields.characters[2].bonus[6] = 32767;
    fields.characters[3].base[10] = -417;
    fields.characters[3].bonus[10] = 731;

    const Party fromBytes = mm6::stageResistanceParty(
        baseline, resistanceBytes(fields), GameTitle::MM6);
    const Party fromFields = mm6::stageResistanceParty(baseline, fields, GameTitle::MM6);
    EXPECT_EQ(mm6::captureResistanceFields(fromBytes, GameTitle::MM6), fields);
    EXPECT_EQ(mm6::captureResistanceFields(fromFields, GameTitle::MM6), fields);
    EXPECT_EQ(fromFields.uNumGold, 77);
    EXPECT_EQ(mm6::captureResistanceFields(baseline, GameTitle::MM6), before);
    EXPECT_THROW((void) mm6::stageResistanceParty(baseline, fields, GameTitle::MM7),
                 std::exception);
    EXPECT_EQ(mm6::captureResistanceFields(baseline, GameTitle::MM6), before);
}

UNIT_TEST(MM6DecodedEffectPartyStage, BuffFieldsMatchV1ByteStage) {
    Party baseline;
    baseline.uNumGold = 89;
    const auto before = mm6::captureBuffFields(baseline, GameTitle::MM6);
    auto fields = before;
    fields.party[7] = {123456, 12, 3, 50, -1, true};
    fields.characters[2][13] = {98765, 23, 2, 0, 2, false};
    fields.characters[3][23] = {-1, 0, 0, 0, -1, false};

    const Party fromBytes = mm6::stageBuffParty(baseline, buffBytes(fields), GameTitle::MM6);
    const Party fromFields = mm6::stageBuffParty(baseline, fields, GameTitle::MM6);
    EXPECT_EQ(mm6::captureBuffFields(fromBytes, GameTitle::MM6), fields);
    EXPECT_EQ(mm6::captureBuffFields(fromFields, GameTitle::MM6), fields);
    EXPECT_EQ(fromFields.uNumGold, 89);
    EXPECT_EQ(mm6::captureBuffFields(baseline, GameTitle::MM6), before);
    EXPECT_THROW((void) mm6::stageBuffParty(baseline, fields, GameTitle::MM7),
                 std::exception);
}

UNIT_TEST(MM6DecodedEffectPartyStage, LateInvalidBuffRejectsBeforePartyCopy) {
    Party baseline;
    baseline.uNumGold = 91;
    const auto before = mm6::captureBuffFields(baseline, GameTitle::MM6);
    auto fields = before;
    fields.party[0] = {123456, 12, 3, 50, -1, true};
    fields.characters[0][0] = {98765, 23, 2, 0, 2, false};
    fields.characters[3][23].caster = 4; // Last physical record must fail preflight.
    EXPECT_THROW((void) mm6::stageBuffParty(baseline, fields, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(baseline.uNumGold, 91);
    EXPECT_EQ(mm6::captureBuffFields(baseline, GameTitle::MM6), before);

    fields.characters[3][23].caster = -1;
    fields.characters[3][23].overlayId = 51;
    EXPECT_THROW((void) mm6::stageBuffParty(baseline, fields, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(mm6::captureBuffFields(baseline, GameTitle::MM6), before);
}
