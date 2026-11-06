#include "Engine/Evt/MM6ConditionVariables.h"

#include <stdexcept>

#include "Engine/Objects/Character.h"
#include "Engine/Objects/CharacterConditions.h"

namespace mm6 {

bool isConditionVariable(int rawId) {
    return rawId >= 87 && rawId <= 104;
}

Condition checkedCondition(EvtOpcode opcode, int rawId, int value) {
    // Values are a subset of operand forms in the F3-matched shipped EVT set.
    // Only Cursed, Fear and Drunk can be applied through the current runtime
    // without MM7 protection/item blocking. Other status writes remain
    // guarded until their MM6 blocking and health effects are established.
    bool supported = false;
    switch (opcode) {
        case EVENT_Compare:
            supported = (rawId == 87 && (value == 0 || value == 1)) ||
                        (rawId == 90 && value == 10);
            break;
        case EVENT_Add:
            supported = (rawId == 87 && value == 1) ||
                        (rawId == 90 && (value == 1 || value == 2 || value == 10)) ||
                        (rawId == 91 && value == 5);
            break;
        case EVENT_Subtract:
            supported = rawId == 90 && value == 10;
            break;
        case EVENT_Set:
            supported = ((rawId == 87 || rawId == 104) && value == 0) ||
                        (rawId == 91 && (value == 0 || value == 1));
            break;
        default:
            break;
    }
    if (!supported)
        throw std::runtime_error("MM6 condition variable operation or operand is unproved");

    // MM6 has seventeen named ailments followed by Good. Runtime slot 17 is
    // MM7-only Zombie, so Good must be mapped explicitly.
    return rawId == 104 ? CONDITION_GOOD : static_cast<Condition>(rawId - 87);
}

bool compareCondition(const CharacterConditions &conditions, int rawId, int value) {
    return conditions.has(checkedCondition(EVENT_Compare, rawId, value));
}

void addCondition(Character &character, int rawId, int value) {
    character.SetCondition(checkedCondition(EVENT_Add, rawId, value), 1);
    character.PlayAwardSound_Anim97();
}

void subtractCondition(Character &character, int rawId, int value) {
    character.ResetCondition(checkedCondition(EVENT_Subtract, rawId, value));
    character.PlayAwardSound_AnimSubtract();
}

void setCondition(Character &character, int rawId, int value) {
    Condition condition = checkedCondition(EVENT_Set, rawId, value);
    if (condition == CONDITION_GOOD)
        character.conditions.resetAll();
    else
        character.SetCondition(condition, 1);
    character.PlayAwardSound_Anim();
}

} // namespace mm6
