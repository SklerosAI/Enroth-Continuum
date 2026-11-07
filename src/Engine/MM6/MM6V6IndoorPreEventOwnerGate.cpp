#include "MM6V6IndoorPreEventOwnerGate.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

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
        if (live.vPosition != native.vPosition.toFloat() ||
            live._yawAngle != native._yawAngle || live.uEventID != native.uEventID ||
            live.uTriggerRange != native.uTriggerRange)
            throw std::runtime_error("MM6 v6 pre-event decoration identity differs at slot " +
                                     std::to_string(index));
    }
}

} // namespace mm6
