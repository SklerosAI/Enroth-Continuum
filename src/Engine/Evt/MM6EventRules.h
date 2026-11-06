#pragma once

#include "Engine/Objects/CombinedSkillValue.h"

// MM6 encodes a Novice CheckSkill requirement as zero. It is a minimum
// proficiency check, so higher mastery still satisfies the level threshold.
// Other mastery encodings retain their existing exact-match rule until a
// title-specific source or active event requires a broader ordering rule.
inline bool mm6CheckSkillSatisfied(CombinedSkillValue value, Mastery encodedMastery, int minimumLevel) {
    if (encodedMastery == MASTERY_NONE)
        return value.mastery() != MASTERY_NONE && value.level() >= minimumLevel;
    return value.mastery() == encodedMastery && value.level() >= minimumLevel;
}
