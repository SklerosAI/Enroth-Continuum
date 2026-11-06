#include "MM6RecoverySaveFields.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "Engine/Engine.h"
#include "Engine/Party.h"

#include "GUI/UI/UIRest.h"

namespace mm6 {
namespace {

void requireSafeMM6State(const Party &party, GameTitle title) {
    if (title != GameTitle::MM6 || !engine || engine->gameTitle() != GameTitle::MM6)
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

RecoverySaveFields captureRecoveryFields(const Party &party, GameTitle title) {
    requireSafeMM6State(party, title);
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
    (void) encodeRecoverySection(fields);
    return fields;
}

Party stageRecoveryParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireSafeMM6State(live, title);
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
    setDerivedCalendar(&staged, calendar);
    return staged;
}

} // namespace mm6
