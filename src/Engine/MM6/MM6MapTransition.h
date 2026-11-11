#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/MapEnums.h"
#include "Engine/PartyPlacement.h"

#include "Utility/Memory/Blob.h"

#include "MM6ParkedMapSection.h"

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
 * Applies the MM6 border confirmation to the live party and starts the change to the neighbouring map: the map is
 * autosaved, the trip's days pass with rest, food is eaten, a hungry party gets Weak, and the party arrives at the
 * wrapped position.
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
 * Bookkeeping every MM6 map change does before the target map loads. The map the party leaves is parked for the
 * session: its delta with the live chests, doors, visibility and reset counters, and its actors, ground items and event
 * variables.
 *
 * @param target                        Map the party is about to enter.
 */
void leaveMap(MapId target);

/**
 * @param deltaName                     Delta file of the map being loaded, e.g. `oute3.ddm`.
 * @return                              The decoded delta the map was parked with, to load in place of the one in
 *                                      `pMapDeltas`, or empty for a map the session hasn't left.
 */
[[nodiscard]] std::optional<Blob> parkedMapDelta(std::string_view deltaName);

/**
 * Puts back the actors, ground items and event variables of a parked map once its parked delta has loaded, and
 * forgets the parked map. A respawned map keeps what the respawn made.
 *
 * @param deltaName                     Delta file of the map that just loaded.
 * @param respawned                     Whether the load respawned the map.
 */
void restoreParkedMap(std::string_view deltaName, bool respawned);

/**
 * Forgets every parked map, for a new or loaded game.
 */
void forgetParkedMaps();

/**
 * A map of the session as a save holds it, its native delta next to the lists the delta can't hold.
 */
struct SessionMap {
    std::string deltaName; // Delta file of the map, e.g. `oute3.ddm`.
    Blob delta; // Decoded delta.
    ParkedMapRuntime runtime;
};

/**
 * @return                              Every parked map, and the active map parked the way leaving it now would park
 *                                      it, sorted by map name. Nothing changes. Throws when the active map can't be
 *                                      parked.
 */
[[nodiscard]] std::vector<SessionMap> captureSessionMaps();

/**
 * Replaces the parked maps with the maps of a loaded save. The active map is among them, so the load that follows
 * restores it the way a return in the same session would.
 *
 * @param maps                          Maps of the save, at most one per map.
 */
void restoreSessionMaps(std::vector<SessionMap> maps);

} // namespace mm6
