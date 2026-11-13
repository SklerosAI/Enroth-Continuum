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

/**
 * @offset 0x42F31F (MM6)
 *
 * @param hour                          Hour of the day the party rents the room, 0 to 23.
 * @param minute                        Minute of that hour.
 * @return                              Minutes the party sleeps in a rented room. It wakes at 6 am.
 */
int tavernRoomRestMinutes(int hour, int minute);

} // namespace mm6
