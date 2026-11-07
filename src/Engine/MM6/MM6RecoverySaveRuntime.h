#pragma once

#include <cstdint>

#include "MM6RecoverySaveFields.h"
#include "Engine/MapEnums.h"

namespace mm6 {

// Apply an already decoded v2 RCVR owner to a detached Party copy after PART
// set the playing clock. The v1 byte path delegates here after decoding.
Party stageRecoveryParty(const Party &live, const RecoverySaveFields &fields, GameTitle title);

struct PostWorldArmageddonResumePlan {
    LevelType levelType = LEVEL_NULL;
    std::int64_t playingTicks = 0;
    std::int64_t timerTicks = 0;
    std::int32_t damage = 0;
    std::int32_t forceCount = 0;
};

// DoPrepareWorld clears Armageddon's timer after RCVR has staged the Party.
// Check the final map and coupled fields before returning only the timer
// restoration intent. This does not write a live Party.
PostWorldArmageddonResumePlan preparePostWorldArmageddonResume(
    const Party &postWorld, const RecoverySaveFields &saved,
    LevelType finalLevel, GameTitle title);

// After a future all-owner publisher restores the timer, verify the exact
// tuple before the first input frame. This remains read-only.
void requirePostWorldArmageddonResumeAgreement(
    const Party &finalParty, const PostWorldArmageddonResumePlan &plan,
    LevelType finalLevel, GameTitle title);

} // namespace mm6
