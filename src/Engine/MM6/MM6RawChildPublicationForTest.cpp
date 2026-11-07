#include "MM6RawChildPublicationForTest.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <variant>

#include "MM6ChestSaveFields.h"
#include "MM6NpcSaveFields.h"
#include "MM6PersistentEventBoundary.h"
#include "MM6RawCursorShopStage.h"
#include "MM6RawPartyInventoryStage.h"
#include "MM6RawSessionStage.h"
#include "MM6RawV2Admission.h"
#include "MM6SavedActorPidPolicy.h"

namespace mm6::testing {
namespace {

void requireMatchingMapCache(const StagedRawSessionFields &staged) {
    const auto &saved = staged.raw.saved.maps.deltas;
    const auto &cache = staged.world.world.mapDeltas;
    if (saved.size() != cache.size())
        throw std::runtime_error("MM6 child map cache count differs from v2 MAPS");
    for (const SavedMapDelta &delta : saved) {
        const auto found = cache.find(delta.name);
        if (found == cache.end() || found->second.size() != delta.bytes.size() ||
            (found->second.size() && std::memcmp(found->second.data(), delta.bytes.data(),
                                                  delta.bytes.size()) != 0))
            throw std::runtime_error("MM6 child map cache differs from v2 MAPS");
    }
}

void requireMatchingVisuals(const StagedRawSessionFields &staged) {
    const auto &saved = staged.raw.saved.metadata;
    const auto &visuals = staged.world.visuals;
    if (visuals.timer.time().ticks() != saved.timer.elapsedTicks)
        throw std::runtime_error("MM6 child timer differs from v2 TIME");
    for (std::size_t index = 0; index < saved.overlays.size(); ++index) {
        const SaveOverlayField &source = saved.overlays[index];
        const ActiveOverlay &live = visuals.overlays.pOverlays[index];
        if (live.indexToOverlayTable != source.tableIndex ||
            live.spriteFrameTime != source.frameTime ||
            live.animLength != source.animationLength ||
            live.screenSpaceX != source.x || live.screenSpaceY != source.y ||
            live.pid.packed() != source.packedPid || live.projSize != source.projectedSize ||
            live.fpDamageMod != source.damageModifier)
            throw std::runtime_error("MM6 child overlays differ from v2 VISU");
    }
}

void requireMatchingWorld(const StagedRawSessionFields &staged, GameTitle title) {
    const auto &saved = staged.raw.saved;
    requireMatchingMapCache(staged);
    if (encodeNpcSection(captureNpcFields(staged.world.world.npcs, title)).payload !=
        encodeNpcSection(saved.npcs).payload)
        throw std::runtime_error("MM6 child NPC owner differs from v2 RESC");
    requireMatchingVisuals(staged);
    if (captureChestSaveFields(staged.world.chestTitle, saved.chest.activeDeltaName,
                               staged.world.rawActiveDelta, title) != saved.chest)
        throw std::runtime_error("MM6 child chest owner differs from v2 CHST");
    const auto freshEvents = stagePersistentEventBoundary(staged.raw.activeMap, title);
    if (freshEvents.mapBytes != staged.persistentEvents.mapBytes ||
        freshEvents.legacy.mapVars != staged.persistentEvents.legacy.mapVars ||
        freshEvents.legacy.decorVars != staged.persistentEvents.legacy.decorVars)
        throw std::runtime_error("MM6 child persistent events differ from active map");
    const bool indoor = std::holds_alternative<StagedIndoorMap>(staged.raw.activeMap);
    if ((indoor && (!staged.indoorPlan || staged.outdoorPlan)) ||
        (!indoor && (staged.indoorPlan || !staged.outdoorPlan)))
        throw std::runtime_error("MM6 child active map plan is missing or ambiguous");
    const std::string_view mapName = indoor ? staged.indoorPlan->mapName : staged.outdoorPlan->mapName;
    if (mapName != saved.metadata.header.mapFileName)
        throw std::runtime_error("MM6 child active map plan differs from v2 HEAD");
}

} // namespace

RawChildPublicationAdmission inspectRawChildPublication(
    const StagedRawSessionFields &staged, const ItemTable &itemTable, GameTitle title) {
    if (title != GameTitle::MM6 || !staged.raw.inventory.ready ||
        staged.raw.saved.inventory != staged.raw.inventory ||
        staged.party.raw != staged.raw.inventory)
        throw std::runtime_error("MM6 child inventory owners differ from v2 INVR");
    if (captureRawPartyInventories(staged.party, itemTable, title) != staged.raw.inventory)
        throw std::runtime_error("MM6 child Party inventory differs from physical INVR");
    validateMetadataAgainstCore(staged.raw.saved.metadata, staged.raw.saved.core);
    const int activeCharacter = staged.party.party.hasActiveCharacter() ?
        staged.party.party.activeCharacterIndex() : -1;
    if (activeCharacter != staged.raw.saved.metadata.ui.activeCharacter)
        throw std::runtime_error("MM6 child Party active character differs from v2 UIST");
    requireRawV2NoBeacons(staged.party.party, title);
    requireNoActiveSavedActorOverlays(staged.raw.saved.metadata.overlays, title);
    validateRawCursorShopJoin(staged.raw.saved, staged.raw.cursor, staged.raw.shops, title);
    RawChildPublicationAdmission admission;
    admission.cursorImage = stageRawCursorImagePlan(
        staged.party.party, staged.raw.cursor, itemTable, title);
    requireMatchingWorld(staged, title);

    if (staged.needsRecovery)
        admission.blockers.push_back(RawChildPublicationBlocker::BackupNeedsRecovery);
    if (admission.cursorImage.occupied && !admission.cursorImage.pickedItemOffset)
        admission.blockers.push_back(RawChildPublicationBlocker::HeldItemOffsetUnresolved);
    admission.blockers.push_back(RawChildPublicationBlocker::MouseAssetNotResolved);
    admission.blockers.push_back(RawChildPublicationBlocker::NpcStaticResourcesNotBound);
    admission.blockers.push_back(RawChildPublicationBlocker::ActiveWorldResourcesNotBound);
    admission.blockers.push_back(RawChildPublicationBlocker::ChestRuntimeCollectionNotJoined);
    if (staged.raw.activeCounts.actors || staged.raw.activeCounts.sprites ||
        staged.raw.activeCounts.chests)
        admission.blockers.push_back(RawChildPublicationBlocker::RebuiltObjectIdentityUnchecked);
    admission.blockers.push_back(RawChildPublicationBlocker::PersistentEventsNotCheckedAfterWorldLoad);
    admission.blockers.push_back(RawChildPublicationBlocker::BeaconAndIndoorFaceOwnersNotPublished);
    admission.blockers.push_back(RawChildPublicationBlocker::EventTimersAndTriggersNotReset);
    admission.blockers.push_back(RawChildPublicationBlocker::TransientUiAndActionsNotReset);
    admission.blockers.push_back(RawChildPublicationBlocker::PostWorldAdmissionHookMissing);
    return admission;
}

} // namespace mm6::testing
