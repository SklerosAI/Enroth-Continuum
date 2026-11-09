#pragma once

namespace mm6 {

/**
 * @param value                         The tavern's `Val` column in 2DEvents.txt.
 * @param merchant                      Character's MM6 merchant percentage.
 * @return                              Gold the tavern asks for a room, never below a third of the full price.
 */
int tavernRoomPrice(float value, int merchant);

/**
 * @param value                         The tavern's `Val` column in 2DEvents.txt.
 * @param merchant                      Character's MM6 merchant percentage.
 * @return                              Gold the tavern asks to fill the party's packs, never below a third of the full
 *                                      price.
 */
int tavernFoodPrice(float value, int merchant);

} // namespace mm6
