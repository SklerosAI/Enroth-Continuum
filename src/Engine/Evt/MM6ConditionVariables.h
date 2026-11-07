#pragma once

#include "Core/Time/Time.h"

#include "Engine/Objects/CharacterEnums.h"

class CharacterConditions;

namespace mm6 {

/**
 * @param rawId                         MM6 EVT variable ID.
 * @return                              Whether `rawId` is one of the seventeen MM6 conditions (87-103) or Good (104).
 *                                      None of the four operations on them reads its operand.
 */
bool isConditionVariable(int rawId);

/**
 * @param rawId                         MM6 condition variable ID.
 * @return                              Condition it addresses. ID 104 is `CONDITION_GOOD`, skipping the MM7-only
 *                                      Zombie slot.
 */
Condition eventCondition(int rawId);

/**
 * @param conditions                    Conditions of the selected character.
 * @param rawId                         MM6 condition variable ID. Good isn't compared by any shipped script and throws.
 * @return                              Whether the character has the condition.
 */
bool compareCondition(const CharacterConditions &conditions, int rawId);

/**
 * Set and Add. Starts the condition at `now`, with no protection check and no health or mana change. Good clears
 * every condition.
 *
 * @param conditions                    Conditions of the selected character.
 * @param rawId                         MM6 condition variable ID.
 * @param now                           Current playing time.
 */
void startCondition(CharacterConditions &conditions, int rawId, Time now);

/**
 * Subtract. Clears the condition.
 *
 * @param conditions                    Conditions of the selected character.
 * @param rawId                         MM6 condition variable ID.
 * @return                              Whether the command had an effect. Subtracting Good does nothing, not even
 *                                      the sound.
 */
bool clearCondition(CharacterConditions &conditions, int rawId);

} // namespace mm6
