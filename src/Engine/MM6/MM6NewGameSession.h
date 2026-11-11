#pragma once

namespace mm6 {

/**
 * Stages a freshly created MM6 party the way a load of its new-game save would: native records for the creation
 * items, the default map deltas, a new chest seed and the empty shops, all checked as one first-map v2 save in
 * memory. Publishes the staged owners and admits the first map's chest load. Called after party creation and before
 * the first map loads. When the party can't be staged, only the map deltas and the chest seed are published, the
 * first map still loads, and saving stays refused for the session.
 *
 * @return                              Whether the party was staged. Never throws.
 */
bool startNewGameSession();

/**
 * Arms manual saving for the staged new-game session with a first-map anchor captured from the world the first map
 * just prepared, the way a first-map load arms it with the anchor of its slot. Called after world preparation and
 * before the first input frame. Without a staged session nothing changes. A refused capture leaves saving refused.
 */
void armNewGameSessionAtFirstMap();

} // namespace mm6
