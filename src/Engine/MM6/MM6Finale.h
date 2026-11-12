#pragma once

#include <optional>

#include "Engine/Data/HouseEnums.h"

class Actor;

namespace mm6 {

/**
 * Endings of the original's ending routine, numbered as its argument.
 */
enum class FinaleEnding {
    FINALE_ENDING_WIN = 0, // House 600, the party escaped the hive with the Ritual of the Void.
    FINALE_ENDING_LOSE = 1, // House 601, the reactor blew up and took the world with it.
};
using enum FinaleEnding;

/**
 * @param houseId                       House a script or the engine is entering.
 * @return                              Ending that entering `houseId` plays, `std::nullopt` for any other house.
 */
std::optional<FinaleEnding> finaleEndingForHouse(HouseId houseId);

/**
 * Plays `ending` on the next message pass, the way entering its house does.
 *
 * @param ending                        Ending to play.
 */
void queueFinaleEnding(FinaleEnding ending);

/**
 * MM6 part of `Actor::Die` for the hive bosses. The Demon Queen sets quest bit 202. The reactor loses the game at once
 * when nobody carries the Ritual of the Void. Otherwise it sets quest bit 180, summons the hive guards, moves five
 * doors, places the party at the hive exit and clears every condition.
 *
 * @param actor                         Monster that has just died.
 */
void onFinaleMonsterDeath(const Actor &actor);

/**
 * Removes the Demon Queen and the reactor from a freshly loaded map once quest bits 202 and 180 say they are dead.
 */
void removeDefeatedFinaleBosses();

/**
 * @return                              Whether the party is in the hive after the reactor blew and before it escaped
 *                                      with the Ritual of the Void. Party death and leaving the map lose the game then,
 *                                      and autosave is skipped.
 */
bool isHiveFinaleUnderway();

/**
 * Plays the movies of `ending` and opens its screen.
 *
 * @param ending                        Ending to play.
 */
void playFinaleEnding(FinaleEnding ending);

/**
 * Finishes the ending once its screen is closed. A won game goes on in New Sorpigal, a lost one quits to the main menu.
 */
void closeFinaleEnding();

/**
 * Plays the losing ending in place of a map change that leaves the hive while `isHiveFinaleUnderway`.
 *
 * @return                              Whether the map change was dropped for the ending.
 */
bool loseOnLeavingHive();

} // namespace mm6
