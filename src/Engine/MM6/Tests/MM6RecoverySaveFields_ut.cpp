#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Core/Time/Time.h"

#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/MM6/MM6RecoverySaveFields.h"

namespace {

mm6::RecoverySaveFields example() {
    mm6::RecoverySaveFields fields;
    fields.playingTicks = Time(0, 0, 9).ticks();
    fields.lastRegenerationTicks = fields.playingTicks - 100;
    fields.daysWithoutRest = 2;
    fields.characterRecoveryTicks = {0, 127, 640, 1};
    fields.turnBasedRecoveryTicks = {0, 0, 36, 0};
    fields.daily[0] = {2, 1, 0};
    fields.daily[3] = {3, 4, 127};
    return fields;
}

std::vector<std::uint8_t> saveBytes(const mm6::RecoverySaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeRecoverySection(fields)}});
}

} // namespace

UNIT_TEST(MM6RecoverySaveFields, RoundTripClockAndDerivedCalendar) {
    mm6::RecoverySaveFields fields = example();
    EXPECT_EQ(mm6::encodeRecoverySection(fields).payload.size(), 100);
    std::vector<std::uint8_t> bytes = saveBytes(fields);
    EXPECT_EQ(mm6::decodeRecoverySection(mm6::decodeSaveContainer(bytes)), fields);

    mm6::RecoverySaveFields destination;
    mm6::loadRecoveryBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, fields);

    mm6::RecoveryCalendar calendar = mm6::calendarForPlayingTicks(fields.playingTicks);
    EXPECT_EQ(calendar.year, 1168);
    EXPECT_EQ(calendar.month, 0);
    EXPECT_EQ(calendar.week, 0);
    EXPECT_EQ(calendar.day, 0);
    EXPECT_EQ(calendar.hour, 9);
    EXPECT_EQ(calendar.minute, 0);

    mm6::CoreSaveFields core;
    core.party.playingTicks = fields.playingTicks;
    EXPECT_NO_THROW(mm6::validateRecoveryAgainstCore(fields, core));
    ++core.party.playingTicks;
    EXPECT_THROW(mm6::validateRecoveryAgainstCore(fields, core), std::exception);
}

UNIT_TEST(MM6RecoverySaveFields, RejectsInvalidBeforeMutation) {
    const mm6::RecoverySaveFields before = example();
    mm6::RecoverySaveFields destination = before;

    mm6::RecoverySaveFields invalid = before;
    invalid.lastRegenerationTicks = invalid.playingTicks + 1;
    EXPECT_THROW((void) mm6::encodeRecoverySection(invalid), std::exception);
    invalid = before;
    invalid.characterRecoveryTicks[2] = -1;
    EXPECT_THROW((void) mm6::encodeRecoverySection(invalid), std::exception);
    invalid = before;
    invalid.daily[3].fireSpike = 128;
    EXPECT_THROW((void) mm6::encodeRecoverySection(invalid), std::exception);

    mm6::SaveContainer container{{mm6::encodeRecoverySection(before)}};
    container.sections[0].payload[21] = 1;
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeRecoverySection(before)}};
    container.sections[0].payload.pop_back();
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeRecoverySection(before)}};
    container.sections[0].required = false;
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    std::vector<std::uint8_t> wrongTitle = saveBytes(before);
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(wrongTitle, &destination), std::exception);
    EXPECT_EQ(destination, before);
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(saveBytes(before), nullptr), std::exception);
}
