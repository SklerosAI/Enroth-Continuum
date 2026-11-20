#pragma once

#include "Engine/MapEnums.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/Objects/Item.h"
#include "Engine/Objects/ItemEnums.h"

struct SpawnPoint;

namespace mm6 {

/**
 * Rolls a random item through MM6.exe's item picker on the MM6 gameplay LCG, and publishes the next LCG state and
 * generated artifact flags. Under the All Artifacts mod the artifact roll skips collected artifacts instead.
 *
 * @param treasureLevel                 Treasure level 1..6.
 * @param requestedKind                 Requested item kind.
 * @return                              Generated item. Throws if the LCG isn't restored or the request is outside the
 *                                      original generator's tables.
 */
Item generateTitleItem(ItemTreasureLevel treasureLevel, RandomItemType requestedKind);

/**
 * Rolls the random ground treasure of one marker, MM6.exe 0x455ED0, on the MM6 gameplay LCG held in `state`. A tier-7
 * roll goes to the direct artifact picker. Otherwise the type roll gives nothing, gold or an item of a random kind
 * 20..43, both at the marker's own level, not the remapped one.
 *
 * @param state                         MM6 title state, must be ready.
 * @param mapTreasureLevel              Treasure level of the map.
 * @param markerLevel                   Treasure level of the marker, 1..7.
 * @return                              Staged item, `ITEM_NULL` for nothing, and the next title state. Throws if the
 *                                      state isn't ready or a level is outside the original tables.
 */
StagedTitleItem stageRandomTreasure(const ChestTitleState &state, MapTreasureLevel mapTreasureLevel,
                                    ItemTreasureLevel markerLevel);

/**
 * Rolls the random ground treasure of one marker with `stageRandomTreasure`, publishes the title state, and puts the
 * item on the ground at the marker.
 *
 * @param mapTreasureLevel              Treasure level of the map.
 * @param spawn                         Treasure spawn point.
 */
void spawnRandomTreasure(MapTreasureLevel mapTreasureLevel, const SpawnPoint &spawn);

} // namespace mm6
