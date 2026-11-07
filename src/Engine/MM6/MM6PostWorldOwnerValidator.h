#pragma once

#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6ChestTitleState.h"
#include "MM6PersistentEventBoundary.h"

struct ItemTable;
struct MapTable;

namespace mm6 {

struct StagedRawSessionFields;

// Explicit read-only final owners at the one-shot post-DoPrepareWorld hook.
// The caller must pass the actual loaded map and process arrays, not copies
// prepared before world reconstruction.
struct PostWorldOwnerView {
    MapId loadedMapId = MAP_INVALID;
    LevelType levelType = LEVEL_NULL;
    const MapTable *maps = nullptr;
    const ItemTable *items = nullptr;
    std::span<const Actor> actors;
    std::span<const SpriteObject> sprites;
    std::span<const Chest> chests;
    std::span<const std::uint8_t> decodedDelta;
    const PersistentVariables *persistent = nullptr;
    const ChestTitleState *chestTitle = nullptr;
};

// Refuse if the saved map, rebuilt actor/sprite slots, event bytes, complete
// chest sidecar, generated chest collection or title-local chest ledger cannot
// be joined. This never mutates a live owner and never admits active VISU.
// Matching slots still do not prove saved-delta provenance or resource/UI
// readiness; those remain separate admission gates.
void requireRawV2PostWorldOwnerAgreement(const StagedRawSessionFields &staged,
                                        const PostWorldOwnerView &live,
                                        GameTitle title);

} // namespace mm6
