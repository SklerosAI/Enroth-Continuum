#include "MM6V6IndoorPreEventOwnerGate.h"

#include <algorithm>
#include <deque>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Snapshots/EntitySnapshots.h"

namespace mm6 {

void requireV6IndoorPreEventOwnerAgreement(
    const StagedV6IndoorSaveFields &staged, std::string_view loadedMapName,
    const std::deque<Actor> &liveActors,
    const std::vector<SpriteObject> &liveSprites,
    const std::vector<LevelDecoration> &liveDecorations, GameTitle title) {
    if (title != GameTitle::MM6 || !staged.represented.indoorPlan ||
        staged.represented.outdoorPlan ||
        loadedMapName != staged.represented.indoorPlan->mapName ||
        staged.saved.raw.metadata.header.mapFileName != loadedMapName)
        throw std::runtime_error("MM6 v6 pre-event owner gate lacks active indoor identity");
    const auto &plan = *staged.represented.indoorPlan;
    if (liveActors.size() != plan.savedDelta.actors.size() ||
        liveActors.size() != staged.saved.actors.count ||
        liveSprites.size() != plan.savedDelta.spriteObjects.size() ||
        liveSprites.size() != staged.saved.sprites.count ||
        liveDecorations.size() != plan.base.decorations.size() ||
        liveDecorations.size() != staged.saved.decorations.count)
        throw std::runtime_error("MM6 v6 pre-event live slot counts differ from BLV/DLV");

    for (std::size_t index = 0; index < liveActors.size(); ++index) {
        const Actor &live = liveActors[index];
        const Actor_MM7 &native = plan.savedDelta.actors[index];
        if (live.id != index || live.npcId != native.npcId ||
            std::to_underlying(live.monsterId) != native.monsterId ||
            std::to_underlying(live.monsterInfo.id) != native.monsterInfo.id ||
            live.initialPosition != native.initialPosition.toFloat())
            throw std::runtime_error("MM6 v6 pre-event actor identity differs at slot " +
                                     std::to_string(index));
    }
    for (std::size_t index = 0; index < liveSprites.size(); ++index) {
        const SpriteObject &live = liveSprites[index];
        const SpriteObject_MM7 &native = plan.savedDelta.spriteObjects[index];
        if (std::to_underlying(live.spriteId) != native.spriteId ||
            live.uObjectDescID != native.objectDescId ||
            live.initialPosition != native.initialPosition.toFloat())
            throw std::runtime_error("MM6 v6 pre-event sprite identity differs at slot " +
                                     std::to_string(index));
    }
    for (std::size_t index = 0; index < liveDecorations.size(); ++index) {
        const LevelDecoration &live = liveDecorations[index];
        const LevelDecoration_MM7 &native = plan.base.decorations[index];
        LevelDecoration reconstructed;
        reconstruct(native, &reconstructed);
        // A zero native yaw uses field_1A's degree fallback in the engine's
        // own reconstruction; the raw _yawAngle word is not the live identity.
        if (live.vPosition != reconstructed.vPosition ||
            live._yawAngle != reconstructed._yawAngle ||
            live.uEventID != reconstructed.uEventID ||
            live.uTriggerRange != reconstructed.uTriggerRange)
            throw std::runtime_error("MM6 v6 pre-event decoration identity differs at slot " +
                                     std::to_string(index));
    }
}

} // namespace mm6
