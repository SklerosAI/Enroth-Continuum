#include "MM6PostRebuildPidSlots.h"

#include <stdexcept>
#include <utility>

#include "MM6NativeActorNpcPolicy.h"

namespace mm6 {
namespace {

bool sameNpcAnchor(int savedNpcId, MonsterId monster, int rebuiltNpcId) {
    if (savedNpcId == rebuiltNpcId)
        return true;
    if (getsStreetNpc(monster, savedNpcId))
        return rebuiltNpcId >= 5000; // The map load replaced the link with a street NPC row.
    return rebuiltNpcId == 0 && clearsNativeActorNpc(monster, savedNpcId);
}

bool sameActorAnchor(const Actor_MM7 &left, const Actor_MM7 &right) {
    return left.npcId == right.npcId && left.monsterId == right.monsterId &&
           left.monsterInfo.id == right.monsterInfo.id &&
           left.initialPosition == right.initialPosition;
}

bool sameActorAnchor(const Actor_MM7 &saved, const Actor &rebuilt, size_t index) {
    return rebuilt.id == index && sameNpcAnchor(saved.npcId, rebuilt.monsterInfo.id, rebuilt.npcId) &&
           saved.monsterId == std::to_underlying(rebuilt.monsterId) &&
           saved.monsterInfo.id == std::to_underlying(rebuilt.monsterInfo.id) &&
           saved.initialPosition.toFloat() == rebuilt.initialPosition;
}

bool sameSpriteAnchor(const SpriteObject_MM7 &left, const SpriteObject_MM7 &right) {
    return left.spriteId == right.spriteId && left.objectDescId == right.objectDescId &&
           left.initialPosition == right.initialPosition;
}

bool sameSpriteAnchor(const SpriteObject_MM7 &saved, const SpriteObject &rebuilt) {
    return saved.spriteId == std::to_underlying(rebuilt.spriteId) &&
           saved.objectDescId == rebuilt.uObjectDescID &&
           saved.initialPosition.toFloat() == rebuilt.initialPosition;
}

void requireSlots(std::string_view savedMapName, std::string_view loadedMapName,
                  std::span<const Actor_MM7> savedActors,
                  std::span<const SpriteObject_MM7> savedSprites,
                  std::span<const Actor_MM7> sidecarActors,
                  std::span<const SpriteObject_MM7> sidecarSprites,
                  std::span<const Actor> actors,
                  std::span<const SpriteObject> sprites,
                  GameTitle title) {
    if (title != GameTitle::MM6 || savedMapName.empty() ||
        savedMapName != loadedMapName)
        throw std::runtime_error("MM6 rebuilt PID slots require the selected saved map");
    if (savedActors.size() != sidecarActors.size() ||
        savedActors.size() != actors.size() ||
        savedSprites.size() != sidecarSprites.size() ||
        savedSprites.size() != sprites.size())
        throw std::runtime_error("MM6 rebuilt PID slot counts differ from saved delta");
    for (size_t i = 0; i < savedActors.size(); ++i)
        if (!sameActorAnchor(savedActors[i], sidecarActors[i]) ||
            !sameActorAnchor(savedActors[i], actors[i], i))
            throw std::runtime_error("MM6 rebuilt actor PID slot differs from saved delta");
    for (size_t i = 0; i < savedSprites.size(); ++i)
        if (!sameSpriteAnchor(savedSprites[i], sidecarSprites[i]) ||
            !sameSpriteAnchor(savedSprites[i], sprites[i]))
            throw std::runtime_error("MM6 rebuilt sprite PID slot differs from saved delta");
}

} // namespace

void requirePostRebuildPidSlotAgreement(
    const DetachedIndoorWorldPlan &plan, std::string_view loadedMapName,
    std::span<const Actor> actors, std::span<const SpriteObject> sprites,
    GameTitle title) {
    requireSlots(plan.mapName, loadedMapName, plan.savedDelta.actors,
                 plan.savedDelta.spriteObjects, plan.activeSidecar.actors,
                 plan.activeSidecar.spriteObjects, actors, sprites, title);
}

void requirePostRebuildPidSlotAgreement(
    const DetachedOutdoorWorldPlan &plan, std::string_view loadedMapName,
    std::span<const Actor> actors, std::span<const SpriteObject> sprites,
    GameTitle title) {
    requireSlots(plan.mapName, loadedMapName, plan.savedDelta.actors,
                 plan.savedDelta.spriteObjects, plan.activeSidecar.actors,
                 plan.activeSidecar.spriteObjects, actors, sprites, title);
}

} // namespace mm6
