#pragma once

#include "Engine/Objects/CharacterEnums.h"

class Character;

namespace mm6 {

/**
 * @param rawId                         MM6 EVT variable ID.
 * @return                              Whether `rawId` is one of the five MM6 resistances (46-50) or one of their
 *                                      bonuses (51-55).
 */
bool isResistanceVariable(int rawId);

/**
 * @param character                     Character to check.
 * @param rawId                         MM6 resistance variable ID.
 * @param value                         Compare operand.
 * @return                              Whether the addressed resistance is at least `value`.
 */
bool compareEventResistance(const Character &character, int rawId, int value);

/**
 * Stores the low byte of `value` in the addressed resistance.
 *
 * @param character                     Character to change.
 * @param rawId                         MM6 resistance variable ID.
 * @param value                         Set operand.
 */
void setEventResistance(Character &character, int rawId, int value);

/**
 * Adds the low word of `value` to the addressed resistance, capping the result at 255.
 *
 * @param character                     Character to change.
 * @param rawId                         MM6 resistance variable ID.
 * @param value                         Add operand.
 */
void addEventResistance(Character &character, int rawId, int value);

/**
 * Subtracts the low word of `value` from the addressed resistance. The result is not floored.
 *
 * @param character                     Character to change.
 * @param rawId                         MM6 resistance variable ID.
 * @param value                         Subtract operand.
 */
void subtractEventResistance(Character &character, int rawId, int value);

/**
 * @param rawId                         MM6 resistance variable ID.
 * @return                              Portrait reaction the game plays after any write to this variable.
 */
SpeechId eventResistanceReaction(int rawId);

} // namespace mm6
