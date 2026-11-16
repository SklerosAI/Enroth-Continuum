#pragma once

#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mm6 {

static constexpr int kFirstTavern = 92; // MM6 house id of A Lonely Knight, the taverns run on to 112.

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

/**
 * @param npcNews                       Contents of MM6's npcnews.txt.
 * @return                              The region of each news entry, from the Map column, in file order.
 */
std::vector<int> tavernNewsRegions(std::string_view npcNews);

/**
 * News a barkeep can tell for a tip. A region is a 1-based position among the first half of the Games.lod directory,
 * where MM6 keeps its base map files, so New Sorpigal is region 40.
 *
 * @offset 0x43BC20 (MM6)
 *
 * @param regions                       Region of each news entry, see `tavernNewsRegions`.
 * @param region                        Region of the map the party is on, 1 when the map isn't in the first half.
 * @return                              The first news entry of the region and how many follow it. A region without news
 *                                      tells region 1's.
 */
std::pair<int, int> tavernTipNews(std::span<const int> regions, int region);

} // namespace mm6
