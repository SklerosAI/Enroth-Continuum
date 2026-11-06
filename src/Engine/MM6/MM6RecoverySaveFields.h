#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

struct Party;

namespace mm6 {

struct CoreSaveFields;

struct RecoveryCalendar {
    std::int32_t year = 0;
    std::int32_t month = 0; // Runtime cache is zero-based.
    std::int32_t week = 0;
    std::int32_t day = 0;
    std::int32_t hour = 0;
    std::int32_t minute = 0;
    std::int32_t second = 0;

    bool operator==(const RecoveryCalendar &) const = default;
};

struct RecoveryDailyCounters {
    std::uint8_t divineIntervention = 0;
    std::uint8_t armageddon = 0;
    std::uint8_t fireSpike = 0;

    bool operator==(const RecoveryDailyCounters &) const = default;
};

struct RecoverySaveFields {
    std::int64_t playingTicks = 0; // Must agree with PART/HEAD before commit.
    std::int64_t lastRegenerationTicks = 0;
    std::uint8_t daysWithoutRest = 0;
    std::array<std::int64_t, 4> characterRecoveryTicks{};
    std::array<std::int64_t, 4> turnBasedRecoveryTicks{};
    std::array<RecoveryDailyCounters, 4> daily{};

    bool operator==(const RecoverySaveFields &) const = default;
};

/** Required, exact-size RCVR v1 section for currently represented runtime state. */
SaveSection encodeRecoverySection(const RecoverySaveFields &fields);
RecoverySaveFields decodeRecoverySection(const SaveContainer &container);
void loadRecoveryBeforeMutation(std::span<const std::uint8_t> bytes,
                                RecoverySaveFields *destination);

/** Recompute the current runtime's cached calendar from the clock anchor. */
RecoveryCalendar calendarForPlayingTicks(std::int64_t playingTicks);
void validateRecoveryAgainstCore(const RecoverySaveFields &recovery, const CoreSaveFields &core);

/** Reject active rest/turn mode; stage only on a party copy after core clock staging. */
RecoverySaveFields captureRecoveryFields(const Party &party, GameTitle title);
Party stageRecoveryParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
