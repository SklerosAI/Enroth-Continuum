#pragma once

#include <span>
#include <string>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6SaveContainer.h"

namespace mm6 {

/**
 * The live lists of one map that its MM6 delta can't hold: the actors and ground items, which encounters and drops
 * grow past the delta records, and the event variables.
 */
struct ParkedMapRuntime {
    std::string mapName; // Map file name, e.g. `oute3.odm`.
    std::vector<Actor> actors;
    std::vector<SpriteObject> sprites;
    PersistentVariables variables;
};

/**
 * Encodes the runtime lists of every map of a session into one `PRKM` section.
 *
 * @param maps                          Maps with distinct canonical names, at most one per MM6 map.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Required `PRKM` section. Throws for a value the section can't carry exactly.
 */
SaveSection encodeParkedMapSection(std::span<const ParkedMapRuntime> maps, GameTitle title);

/**
 * @param container                     Container holding exactly one `PRKM` section.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Decoded maps. Throws on a missing, malformed or non-canonical section.
 */
std::vector<ParkedMapRuntime> decodeParkedMapSection(const SaveContainer &container, GameTitle title);

} // namespace mm6
