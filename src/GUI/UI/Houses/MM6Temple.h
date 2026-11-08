#pragma once

class Character;

namespace mm6 {

/**
 * MM6 temples don't read the restriction text in 2DEvents.txt, their limits are hardcoded by house id. Temple Baa
 * can't cure dead, stone or eradicated, Temple Stone cures everything, and every other temple can't cure eradicated.
 *
 * @offset 0x49DAD0 (MM6)
 *
 * @param houseId                       MM6 house id of the temple.
 * @param character                     Character to heal.
 * @return                              Whether the temple offers to heal the character.
 */
bool templeOffersHeal(int houseId, const Character &character);

/**
 * @offset 0x49E910 (MM6)
 *
 * @param character                     Character to heal.
 * @param value                         The temple's `Val` column in 2DEvents.txt.
 * @return                              Gold the temple asks for the heal, at least 1.
 */
int templeHealPrice(const Character &character, float value);

/**
 * Heals a character the way an MM6 temple does. Clears every condition and restores HP and SP, then puts back the
 * conditions this temple can't cure. If one of those was set, HP and SP are put back too. Doesn't check
 * `templeOffersHeal` and doesn't take gold.
 *
 * @offset 0x49E027 (MM6)
 *
 * @param houseId                       MM6 house id of the temple.
 * @param character                     Character to heal.
 */
void templeHeal(int houseId, Character *character);

} // namespace mm6
