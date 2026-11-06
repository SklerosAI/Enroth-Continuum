#include "MM6RecoverySaveFields.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "Engine/Engine.h"
#include "Engine/Party.h"

#include "GUI/UI/UIRest.h"

namespace mm6 {
namespace {

void requireSafeMM6State(const Party &party, GameTitle title, bool requireEngine) {
    if (title != GameTitle::MM6 || (requireEngine && !engine) ||
        (engine && engine->gameTitle() != GameTitle::MM6))
        throw std::runtime_error("MM6 recovery fields cannot apply to another title");
    if (party.bTurnBasedModeOn)
        throw std::runtime_error("MM6 turn-based queue restoration is not implemented");
    if (currentRestType != REST_NONE || remainingRestTime)
        throw std::runtime_error("MM6 active rest UI restoration is not implemented");
}

std::uint8_t captureCounter(char value) {
    if (value < 0)
        throw std::runtime_error("negative MM6 daily spell counter");
    return static_cast<std::uint8_t>(value);
}

void setDerivedCalendar(Party *party, const RecoveryCalendar &calendar) {
    party->uCurrentYear = calendar.year;
    party->uCurrentMonth = calendar.month;
    party->uCurrentMonthWeek = calendar.week;
    party->uCurrentDayOfMonth = calendar.day;
    party->uCurrentHour = calendar.hour;
    party->uCurrentMinute = calendar.minute;
    party->uCurrentTimeSecond = calendar.second;
}

} // namespace

EventClockFields captureEventClocks(const PartyTimeStruct &times, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 event clocks cannot apply to another title");
    EventClockFields fields;
    for (size_t index = 0; index < fields.counters.size(); ++index)
        fields.counters[index] = times.CounterEventValues[index].ticks();
    for (size_t index = 0; index < fields.history.size(); ++index)
        fields.history[index] = times.HistoryEventTimes[index].ticks();
    for (size_t index = 0; index < fields.unknownTimes.size(); ++index)
        fields.unknownTimes[index] = times._s_times[index].ticks();
    RecoverySaveFields probe;
    probe.eventClocks = fields;
    (void) encodeRecoverySection(probe);
    return fields;
}

PartyTimeStruct stageEventClocks(const PartyTimeStruct &live, const EventClockFields &saved,
                                 GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 event clocks cannot apply to another title");
    RecoverySaveFields probe;
    probe.eventClocks = saved;
    (void) encodeRecoverySection(probe);
    PartyTimeStruct staged = live;
    for (size_t index = 0; index < saved.counters.size(); ++index)
        staged.CounterEventValues[index] = Time::fromTicks(saved.counters[index]);
    for (size_t index = 0; index < saved.history.size(); ++index)
        staged.HistoryEventTimes[index] = Time::fromTicks(saved.history[index]);
    for (size_t index = 0; index < saved.unknownTimes.size(); ++index)
        staged._s_times[index] = Time::fromTicks(saved.unknownTimes[index]);
    return staged;
}

RecoverySaveFields captureRecoveryFields(const Party &party, GameTitle title) {
    requireSafeMM6State(party, title, false);
    RecoverySaveFields fields;
    fields.playingTicks = party.playing_time.ticks();
    fields.lastRegenerationTicks = party.last_regenerated.ticks();
    fields.daysWithoutRest = party.days_played_without_rest;
    for (size_t index = 0; index < party.pCharacters.size(); ++index) {
        const Character &character = party.pCharacters[index];
        fields.characterRecoveryTicks[index] = character.timeToRecovery.ticks();
        fields.turnBasedRecoveryTicks[index] = party.pTurnBasedCharacterRecoveryTimes[index].ticks();
        fields.daily[index] = {captureCounter(character.uNumDivineInterventionCastsThisDay),
                               captureCounter(character.uNumArmageddonCasts),
                               captureCounter(character.uNumFireSpikeCasts)};
    }
    fields.eventClocks = captureEventClocks(party.PartyTimes, title);
    fields.armageddonTimerTicks = party.armageddon_timer.ticks();
    fields.armageddonDamage = party.armageddonDamage;
    fields.armageddonForceCount = party.armageddonForceCount;
    (void) encodeRecoverySection(fields);
    return fields;
}

Party stageRecoveryParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireSafeMM6State(live, title, false);
    RecoverySaveFields fields;
    loadRecoveryBeforeMutation(bytes, &fields);
    if (fields.playingTicks != live.playing_time.ticks())
        throw std::runtime_error("MM6 recovery stage requires the core party clock first");
    const RecoveryCalendar calendar = calendarForPlayingTicks(fields.playingTicks);

    Party staged = live;
    staged.last_regenerated = Time::fromTicks(fields.lastRegenerationTicks);
    staged.days_played_without_rest = fields.daysWithoutRest;
    for (size_t index = 0; index < staged.pCharacters.size(); ++index) {
        Character &character = staged.pCharacters[index];
        character.timeToRecovery = Duration::fromTicks(fields.characterRecoveryTicks[index]);
        staged.pTurnBasedCharacterRecoveryTimes[index] = Duration::fromTicks(fields.turnBasedRecoveryTicks[index]);
        character.uNumDivineInterventionCastsThisDay = static_cast<char>(fields.daily[index].divineIntervention);
        character.uNumArmageddonCasts = static_cast<char>(fields.daily[index].armageddon);
        character.uNumFireSpikeCasts = static_cast<char>(fields.daily[index].fireSpike);
    }
    staged.PartyTimes = stageEventClocks(live.PartyTimes, fields.eventClocks, title);
    staged.armageddon_timer = Duration::fromTicks(fields.armageddonTimerTicks);
    staged.armageddonDamage = fields.armageddonDamage;
    staged.armageddonForceCount = fields.armageddonForceCount;
    setDerivedCalendar(&staged, calendar);
    return staged;
}

} // namespace mm6
