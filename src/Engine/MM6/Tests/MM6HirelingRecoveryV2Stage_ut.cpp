#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/Party.h"

namespace {

mm6::HirelingSaveFields hirelings() {
    mm6::HirelingSaveFields fields;
    fields.hirelings[0].name = "Companion";
    fields.hirelings[0].portraitId = 653;
    fields.hirelings[0].house = HOUSE_MM6_LAST;
    fields.hirelings[0].profession = NPC_PROFESSION_MM6_LAST;
    fields.displayNames[0] = "Companion display";
    fields.sacrifice[0] = {true, 7, 100};
    (void) mm6::encodeHirelingSection(fields);
    return fields;
}

mm6::RecoverySaveFields recovery(std::int64_t playingTicks) {
    mm6::RecoverySaveFields fields;
    fields.playingTicks = playingTicks;
    fields.lastRegenerationTicks = playingTicks - 100;
    fields.daysWithoutRest = 2;
    fields.characterRecoveryTicks = {0, 127, 640, 1};
    fields.turnBasedRecoveryTicks = {0, 0, 36, 0};
    fields.daily[0] = {2, 1, 0};
    fields.eventClocks.counters[2] = 314;
    fields.eventClocks.history[17] = -1;
    fields.armageddonTimerTicks = 256;
    fields.armageddonDamage = 17;
    fields.armageddonForceCount = -4;
    (void) mm6::encodeRecoverySection(fields);
    return fields;
}

std::vector<std::uint8_t> hirelingBytes(const mm6::HirelingSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeHirelingSection(fields)}});
}

std::vector<std::uint8_t> recoveryBytes(const mm6::RecoverySaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeRecoverySection(fields)}});
}

UNIT_TEST(MM6HirelingRecoveryV2Stage, HirelingDecodedFieldsMatchV1Bytes) {
    auto baseline = std::make_unique<Party>();
    baseline->pHireling1Name = "Prior display";
    baseline->pHirelings[0].name = "Prior companion";
    baseline->pHirelingsSacrifice[0].elapsedTime = Duration::fromTicks(9);
    const mm6::HirelingSaveFields fields = hirelings();

    Party fromV1 = mm6::stageHirelingParty(*baseline, hirelingBytes(fields), GameTitle::MM6);
    Party fromV2 = mm6::stageHirelingParty(*baseline, fields, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeHirelingSection(mm6::captureHirelingFields(fromV1, GameTitle::MM6)),
              mm6::encodeHirelingSection(mm6::captureHirelingFields(fromV2, GameTitle::MM6)));
    EXPECT_EQ(fromV2.pHirelings[0].name, "Companion");
    EXPECT_EQ(fromV2.pHireling1Name, "Companion display");
    EXPECT_EQ(fromV2.pHirelingsSacrifice[0].endTime.ticks(), 100);
    EXPECT_EQ(baseline->pHirelings[0].name, "Prior companion");
    EXPECT_EQ(baseline->pHireling1Name, "Prior display");
    EXPECT_EQ(baseline->pHirelingsSacrifice[0].elapsedTime.ticks(), 9);

    mm6::HirelingSaveFields invalid = fields;
    invalid.sacrifice[0].endTicks = 0;
    EXPECT_THROW((void) mm6::stageHirelingParty(*baseline, invalid, GameTitle::MM6), std::exception);
    invalid = fields;
    invalid.displayNames[0] = std::string(1025, 'x');
    EXPECT_THROW((void) mm6::stageHirelingParty(*baseline, invalid, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageHirelingParty(*baseline, fields, GameTitle::MM7), std::exception);
    EXPECT_EQ(baseline->pHirelings[0].name, "Prior companion");
    EXPECT_EQ(baseline->pHireling1Name, "Prior display");
}

UNIT_TEST(MM6HirelingRecoveryV2Stage, RecoveryDecodedFieldsMatchV1Bytes) {
    auto baseline = std::make_unique<Party>();
    baseline->playing_time = Time(0, 0, 9);
    baseline->last_regenerated = Time::fromTicks(3);
    baseline->armageddonDamage = 99;
    const mm6::RecoverySaveFields fields = recovery(baseline->playing_time.ticks());

    Party fromV1 = mm6::stageRecoveryParty(*baseline, recoveryBytes(fields), GameTitle::MM6);
    Party fromV2 = mm6::stageRecoveryParty(*baseline, fields, GameTitle::MM6);
    EXPECT_EQ(mm6::captureRecoveryFields(fromV1, GameTitle::MM6),
              mm6::captureRecoveryFields(fromV2, GameTitle::MM6));
    EXPECT_EQ(fromV2.pCharacters[2].timeToRecovery.ticks(), 640);
    EXPECT_EQ(fromV2.PartyTimes.CounterEventValues[2].ticks(), 314);
    EXPECT_EQ(fromV2.armageddonDamage, 17);
    EXPECT_EQ(baseline->last_regenerated.ticks(), 3);
    EXPECT_EQ(baseline->armageddonDamage, 99);

    mm6::RecoverySaveFields invalid = fields;
    invalid.daily[0].armageddon = 128;
    EXPECT_THROW((void) mm6::stageRecoveryParty(*baseline, invalid, GameTitle::MM6), std::exception);
    invalid = fields;
    ++invalid.playingTicks;
    EXPECT_THROW((void) mm6::stageRecoveryParty(*baseline, invalid, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageRecoveryParty(*baseline, fields, GameTitle::MM7), std::exception);
    EXPECT_EQ(baseline->last_regenerated.ticks(), 3);
    EXPECT_EQ(baseline->armageddonDamage, 99);
}

} // namespace
