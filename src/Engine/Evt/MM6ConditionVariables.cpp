#include "Engine/Evt/MM6ConditionVariables.h"

#include <stdexcept>

#include "Engine/Objects/CharacterConditions.h"

namespace mm6 {

bool isConditionVariable(int rawId) {
    return rawId >= 87 && rawId <= 104;
}

Condition eventCondition(int rawId) {
    if (!isConditionVariable(rawId))
        throw std::runtime_error("MM6 condition event variable is invalid");
    return rawId == 104 ? CONDITION_GOOD : static_cast<Condition>(rawId - 87);
}

bool compareCondition(const CharacterConditions &conditions, int rawId) {
    Condition condition = eventCondition(rawId);
    if (condition == CONDITION_GOOD)
        throw std::runtime_error("MM6 Good condition comparison is absent from shipped scripts");
    return conditions.has(condition);
}

void startCondition(CharacterConditions &conditions, int rawId, Time now) {
    Condition condition = eventCondition(rawId);
    if (condition == CONDITION_GOOD)
        conditions.resetAll();
    else
        conditions.set(condition, now);
}

bool clearCondition(CharacterConditions &conditions, int rawId) {
    Condition condition = eventCondition(rawId);
    if (condition == CONDITION_GOOD)
        return false;
    conditions.reset(condition);
    return true;
}

} // namespace mm6
