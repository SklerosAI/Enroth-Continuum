#pragma once

#include <span>

class Character;

namespace mm6 {

/**
 * @param characters                    Party members whose inventories are counted, equipped items included.
 * @return                              Circus prize score tested by MM6 EVT variable 224. Items 470, 471 and 477 are
 *                                      worth 1, 3 and 5 points. The item on the cursor doesn't count.
 */
int circusPrizeScore(std::span<const Character> characters);

} // namespace mm6
