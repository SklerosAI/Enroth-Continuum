#pragma once

#include <string_view>

#include "Engine/Objects/CombinedSkillValue.h"
#include "Utility/String/Ascii.h"
#include "Utility/String/Transformations.h"

// MM6 encodes a Novice CheckSkill requirement as zero. It is a minimum
// proficiency check, so higher mastery still satisfies the level threshold.
// Other mastery encodings retain their existing exact-match rule until a
// title-specific source or active event requires a broader ordering rule.
inline bool mm6CheckSkillSatisfied(CombinedSkillValue value, Mastery encodedMastery, int minimumLevel) {
    if (encodedMastery == MASTERY_NONE)
        return value.mastery() != MASTERY_NONE && value.level() >= minimumLevel;
    return value.mastery() == encodedMastery && value.level() >= minimumLevel;
}

inline bool mm6QuestionMatches(std::string_view response, std::string_view answer1, std::string_view answer2) {
    response = trim(response);
    return !response.empty() &&
        (!trim(answer1).empty() && ascii::noCaseEquals(response, trim(answer1)) ||
         !trim(answer2).empty() && ascii::noCaseEquals(response, trim(answer2)));
}
