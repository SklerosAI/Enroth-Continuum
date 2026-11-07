#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Core/Time/Time.h"

#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/MM6/MM6RecoverySaveFields.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/Party.h"

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
    fields.eventClocks.counters[2] = 314;
    fields.eventClocks.history[17] = -1;
    fields.eventClocks.unknownTimes[19] = 2718;
    fields.armageddonTimerTicks = 256;
    fields.armageddonDamage = 17;
    fields.armageddonForceCount = -4; // Runtime decrement can outlast the impulse window.
    return fields;
}

std::vector<std::uint8_t> saveBytes(const mm6::RecoverySaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeRecoverySection(fields)}});
}

} // namespace

UNIT_TEST(MM6RecoverySaveFields, RoundTripClockAndDerivedCalendar) {
    mm6::RecoverySaveFields fields = example();
    EXPECT_EQ(mm6::encodeRecoverySection(fields).payload.size(), 588);
    std::vector<std::uint8_t> bytes = saveBytes(fields);
    EXPECT_EQ(mm6::decodeRecoverySection(mm6::decodeSaveContainer(bytes)), fields);

    mm6::RecoverySaveFields destination;
    mm6::loadRecoveryBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, fields);
    EXPECT_EQ(destination.armageddonTimerTicks, 256);
    EXPECT_EQ(destination.armageddonDamage, 17);
    EXPECT_EQ(destination.armageddonForceCount, -4);

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

UNIT_TEST(MM6RecoverySaveFields, EventClocksCaptureAndStageOnDetachedRuntimeCopy) {
    PartyTimeStruct live;
    live.CounterEventValues[2] = Time::fromTicks(314);
    live.HistoryEventTimes[17] = Time::fromTicks(-1);
    live._s_times[19] = Time::fromTicks(2718);

    mm6::EventClockFields captured = mm6::captureEventClocks(live, GameTitle::MM6);
    EXPECT_EQ(captured.counters[2], 314);
    EXPECT_EQ(captured.history[17], -1);
    EXPECT_EQ(captured.unknownTimes[19], 2718);

    PartyTimeStruct prior;
    prior.CounterEventValues[2] = Time::fromTicks(99);
    PartyTimeStruct staged = mm6::stageEventClocks(prior, captured, GameTitle::MM6);
    EXPECT_EQ(staged.CounterEventValues[2].ticks(), 314);
    EXPECT_EQ(staged.HistoryEventTimes[17].ticks(), -1);
    EXPECT_EQ(staged._s_times[19].ticks(), 2718);
    EXPECT_EQ(prior.CounterEventValues[2].ticks(), 99);

    EXPECT_THROW((void) mm6::captureEventClocks(live, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageEventClocks(prior, captured, GameTitle::MM7), std::exception);
    captured.unknownTimes[19] = -2;
    EXPECT_THROW((void) mm6::stageEventClocks(prior, captured, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior.CounterEventValues[2].ticks(), 99);
}

UNIT_TEST(MM6RecoverySaveFields, CapturesAndStagesArmageddonTupleOnDetachedParty) {
    auto source = std::make_unique<Party>();
    source->playing_time = Time(0, 0, 9);
    source->last_regenerated = source->playing_time;
    source->armageddon_timer = Duration::fromTicks(256);
    source->armageddonDamage = 17;
    source->armageddonForceCount = -4;

    const mm6::RecoverySaveFields fields = mm6::captureRecoveryFields(*source, GameTitle::MM6);
    EXPECT_EQ(fields.armageddonTimerTicks, 256);
    EXPECT_EQ(fields.armageddonDamage, 17);
    EXPECT_EQ(fields.armageddonForceCount, -4);

    auto prior = std::make_unique<Party>();
    prior->playing_time = source->playing_time;
    prior->armageddon_timer = Duration::fromTicks(9);
    prior->armageddonDamage = 99;
    prior->armageddonForceCount = 99;
    Party staged = mm6::stageRecoveryParty(*prior, saveBytes(fields), GameTitle::MM6);
    EXPECT_EQ(staged.armageddon_timer.ticks(), 256);
    EXPECT_EQ(staged.armageddonDamage, 17);
    EXPECT_EQ(staged.armageddonForceCount, -4);
    EXPECT_EQ(prior->armageddon_timer.ticks(), 9);
    EXPECT_EQ(prior->armageddonDamage, 99);
    EXPECT_EQ(prior->armageddonForceCount, 99);
}

UNIT_TEST(MM6RecoverySaveFields, PostWorldArmageddonResumePlanChecksCoupledState) {
    const mm6::RecoverySaveFields fields = example();
    auto postWorld = std::make_unique<Party>();
    postWorld->playing_time = Time::fromTicks(fields.playingTicks);
    postWorld->armageddonDamage = fields.armageddonDamage;
    postWorld->armageddonForceCount = fields.armageddonForceCount;

    const auto plan = mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_OUTDOOR, GameTitle::MM6);
    EXPECT_EQ(plan.timerTicks, fields.armageddonTimerTicks);
    EXPECT_EQ(plan.damage, fields.armageddonDamage);
    EXPECT_EQ(plan.forceCount, fields.armageddonForceCount);
    EXPECT_EQ(postWorld->armageddon_timer.ticks(), 0);
    EXPECT_EQ(postWorld->armageddonDamage, fields.armageddonDamage);
    EXPECT_THROW(mm6::requirePostWorldArmageddonResumeAgreement(
        *postWorld, plan, LEVEL_OUTDOOR, GameTitle::MM6), std::exception);
    postWorld->armageddon_timer = Duration::fromTicks(plan.timerTicks);
    EXPECT_NO_THROW(mm6::requirePostWorldArmageddonResumeAgreement(
        *postWorld, plan, LEVEL_OUTDOOR, GameTitle::MM6));
    EXPECT_THROW(mm6::requirePostWorldArmageddonResumeAgreement(
        *postWorld, plan, LEVEL_INDOOR, GameTitle::MM6), std::exception);
    postWorld->armageddon_timer = 0_ticks;

    EXPECT_THROW((void) mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_INDOOR, GameTitle::MM6), std::exception);
    mm6::RecoverySaveFields quiet = fields;
    quiet.armageddonTimerTicks = 0;
    const auto indoor = mm6::preparePostWorldArmageddonResume(
        *postWorld, quiet, LEVEL_INDOOR, GameTitle::MM6);
    EXPECT_EQ(indoor.timerTicks, 0);
    EXPECT_NO_THROW(mm6::requirePostWorldArmageddonResumeAgreement(
        *postWorld, indoor, LEVEL_INDOOR, GameTitle::MM6));
    EXPECT_THROW((void) mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_OUTDOOR, GameTitle::MM7), std::exception);
    postWorld->armageddonDamage++;
    EXPECT_THROW((void) mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_OUTDOOR, GameTitle::MM6), std::exception);
    postWorld->armageddonDamage--;
    postWorld->armageddonForceCount++;
    EXPECT_THROW((void) mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_OUTDOOR, GameTitle::MM6), std::exception);
    postWorld->armageddonForceCount--;
    postWorld->armageddon_timer = Duration::fromTicks(1);
    EXPECT_THROW((void) mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_OUTDOOR, GameTitle::MM6), std::exception);
    postWorld->armageddon_timer = 0_ticks;
    postWorld->playing_time += Duration::fromTicks(1);
    EXPECT_THROW((void) mm6::preparePostWorldArmageddonResume(
        *postWorld, fields, LEVEL_OUTDOOR, GameTitle::MM6), std::exception);
    EXPECT_EQ(postWorld->armageddon_timer.ticks(), 0);
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
    invalid = before;
    invalid.armageddonTimerTicks = 418;
    EXPECT_THROW((void) mm6::encodeRecoverySection(invalid), std::exception);
    invalid.armageddonTimerTicks = -1;
    EXPECT_THROW((void) mm6::encodeRecoverySection(invalid), std::exception);

    mm6::SaveContainer container{{mm6::encodeRecoverySection(before)}};
    container.sections[0].payload[21] = 1;
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeRecoverySection(before)}};
    container.sections[0].payload[0] = 2; // Superseded v2 payload with a valid checksum.
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);
    container.sections[0].payload.resize(572); // Exact old v2 length.
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeRecoverySection(before)}};
    container.sections[0].payload[572] = 0xa2; // 418 timer ticks with valid container checksums.
    container.sections[0].payload[573] = 1;
    EXPECT_THROW(mm6::loadRecoveryBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeRecoverySection(before)}};
    container.sections[0].payload[571] = 0xff; // Final event-clock byte; the v3 suffix is signed effect state.
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
