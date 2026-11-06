#pragma once

#include "Engine/Evt/EvtEnums.h"
#include "Engine/Objects/CharacterEnums.h"

class Character;
class CharacterConditions;

// The shipped MM6 condition operands have a different ID base from MM7.
// Keep the supported ID, opcode and value combinations narrow until original
// side effects can be checked in live play.
namespace mm6 {

bool isConditionVariable(int rawId);
Condition checkedCondition(EvtOpcode opcode, int rawId, int value);
bool compareCondition(const CharacterConditions &conditions, int rawId, int value);
void addCondition(Character &character, int rawId, int value);
void subtractCondition(Character &character, int rawId, int value);
void setCondition(Character &character, int rawId, int value);

} // namespace mm6
