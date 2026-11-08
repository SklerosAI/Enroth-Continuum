#pragma once

#include <cstdint>
#include <string>

class Character;

namespace mm6 {

/**
 * MM6 training halls don't read the max level text in 2DEvents.txt, their caps are hardcoded by house id.
 *
 * @param houseId                       MM6 house id of the training hall, 79 to 88.
 * @return                              Level the hall stops training at.
 */
int trainingMaxLevel(int houseId);

/**
 * @param level                         Character's current level.
 * @return                              Experience a character needs to train past this level.
 */
int64_t trainingExperience(int level);

/**
 * @param character                     Character to train.
 * @param value                         The hall's `Val` column in 2DEvents.txt.
 * @param merchant                      Character's MM6 merchant percentage.
 * @return                              Gold the hall asks to train the character one level, never below a third of
 *                                      the full price.
 */
int trainingPrice(const Character &character, float value, int merchant);

/**
 * @param houseId                       MM6 house id of the training hall.
 * @param character                     Character to train.
 * @param value                         The hall's `Val` column in 2DEvents.txt.
 * @return                              The hall's train line for the character: the price, the missing experience, or
 *                                      the refusal at the hall's cap.
 */
std::string trainingLabel(int houseId, const Character &character, float value);

/**
 * Trains the active character in an MM6 training hall, with the hall's sounds, the level up speech and the status
 * line. The first level bought in a visit moves time to 9 am, a week after the next dawn, and rests the party.
 *
 * @param houseId                       MM6 house id of the training hall.
 * @param value                         The hall's `Val` column in 2DEvents.txt.
 * @param[in,out] timePassed            Whether this visit already moved time. Set when this call moves it.
 */
void trainActiveCharacter(int houseId, float value, bool *timePassed);

} // namespace mm6
