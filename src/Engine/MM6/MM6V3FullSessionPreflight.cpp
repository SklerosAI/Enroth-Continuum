#include "MM6V3FullSessionPreflight.h"

#include <deque>
#include <stdexcept>
#include <utility>

#include "MM6RawCursorShopStage.h"
#include "MM6RawPartyInventoryStage.h"

namespace mm6 {

V3FullSessionPreflight stageV3FullSessionPreflight(
    std::span<const std::uint8_t> bytes,
    const NpcStaticBaseline &freshNpcs,
    std::span<const std::string> expectedNames,
    const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem,
    const ItemTable &itemTable,
    std::size_t overlayTableCount,
    std::size_t tileTableCount,
    std::size_t runtimeActorCap,
    RandomEngineType selectedRngType,
    GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v3 full-session preflight requires MM6 title");
    V3OutdoorActorMapAgreement actorMap = stageV3OutdoorActorMapAgreement(
        bytes, expectedNames, readBase, verifyItem, runtimeActorCap, title);
    const auto rawBytes = encodeRawSave(actorMap.saved.raw, title);
    StagedRawSessionFields represented = stageRawSessionFields(
        rawBytes, freshNpcs, expectedNames, readBase, verifyItem, itemTable,
        overlayTableCount, tileTableCount, title);
    if (!represented.outdoorPlan || represented.indoorPlan ||
        represented.outdoorPlan->mapName != actorMap.activeMap.mapName ||
        represented.outdoorPlan->savedDelta.actors.size() != actorMap.activeMap.delta.actors.size() ||
        represented.outdoorPlan->activeSidecar.lastRespawnDay != actorMap.activeSidecar.lastRespawnDay ||
        represented.world.rawActiveDelta != actorMap.saved.raw.chest.rawActiveDelta ||
        encodeRawSave(represented.raw.saved, title) != rawBytes)
        throw std::runtime_error("MM6 v3 represented world differs from ACTR map stage");
    validateRawCursorShopJoin(represented.raw.saved, represented.raw.cursor,
                              represented.raw.shops, title);
    if (captureRawPartyInventories(represented.party, itemTable, title) != represented.raw.inventory)
        throw std::runtime_error("MM6 v3 detached Party inventory cannot replay INVR");

    RawCursorImagePlan cursorImage = stageRawCursorImagePlan(
        represented.party.party, represented.raw.cursor, itemTable, title);
    if (cursorImage.occupied || !cursorImage.pickedItemOffset)
        throw std::runtime_error("MM6 v3 occupied cursor lacks a saved pickup offset");

    const std::deque<Actor> actors(actorMap.activeActors.begin(), actorMap.activeActors.end());
    PreEventGeneratedNpcReplay preEvent = stagePreEventGeneratedNpcReplay(
        actorMap.saved.generatedNpcs, actorMap.saved.gameplayRng, actors,
        freshNpcs.stats(), actorMap.activeMap.mapName, selectedRngType, title);

    Party postWorld = represented.party.party;
    postWorld.armageddon_timer = Duration::fromTicks(0);
    PostWorldArmageddonResumePlan armageddon = preparePostWorldArmageddonResume(
        postWorld, actorMap.saved.raw.recovery, LEVEL_OUTDOOR, title);
    return {std::move(actorMap), std::move(represented), std::move(preEvent),
            std::move(cursorImage), armageddon};
}

} // namespace mm6
