#pragma once

#include "Engine/MapEnums.h"
#include "Engine/PartyPlacement.h"

namespace mm6 {

/**
 * Where walking off the edge of the current MM6 outdoor map leads. Also sets the base travel time that
 * `getTravelTime` reduces for hirelings, as the inherited travel dialogue reads it from there.
 *
 * @param partyX                        Party X.
 * @param partyY                        Party Y.
 * @return                              The neighbouring map with the arrival in it, or a default destination when this
 *                                      edge leads nowhere.
 */
[[nodiscard]] MapDestination borderTravelDestination(int partyX, int partyY);

/**
 * Applies the MM6 border confirmation to the live party and starts the change to the neighbouring map: the trip's
 * days pass with rest, food is eaten, a hungry party gets Weak, and the party arrives at the wrapped position.
 *
 * @param offered                       Destination the travel dialogue offered.
 * @return                              Whether the party is leaving. On `false` nothing has changed.
 */
[[nodiscard]] bool confirmBorderTravel(const MapDestination &offered);

/**
 * Puts a border arrival on the target map's terrain, as the original recalculates the height once the map is
 * loaded. Does nothing for any other arrival.
 *
 * @param[in,out] placement             Placement of the map change that just loaded an outdoor map.
 */
void settleBorderArrival(PartyPlacement *placement);

/**
 * Bookkeeping every MM6 map change does before the target map loads.
 *
 * @param target                        Map the party is about to enter.
 */
void leaveMap(MapId target);

} // namespace mm6
