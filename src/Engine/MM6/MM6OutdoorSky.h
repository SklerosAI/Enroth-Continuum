#pragma once

#include <cstdint>
#include <span>
#include <string>

class RandomEngine;

namespace mm6 {

/**
 * Picks the sky bitmap an MM6 outdoor map shows when it loads, the way MM6's outdoor loader does. The sky name in the
 * `.odm` base map plays no part in it.
 *
 * @param deltaBytes                    Decoded `.ddm` delta of the map. Its last 56 bytes start with the last visit
 *                                      time and the sky shown on that visit.
 * @param dayOfMonth                    Current zero-based day of the month.
 * @param rng                           Random engine that rolls a new sky on a new day.
 * @return                              Sky bitmap name. `sky01` on the first visit, the stored sky on a revisit on the
 *                                      same day of the month, and a random `skyNN` otherwise.
 */
std::string outdoorSky(std::span<const std::uint8_t> deltaBytes, int dayOfMonth, RandomEngine *rng);

} // namespace mm6
