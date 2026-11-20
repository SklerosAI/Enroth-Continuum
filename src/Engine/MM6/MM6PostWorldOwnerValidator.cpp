#include "MM6PostWorldOwnerValidator.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/MapTable.h"

#include "MM6ChestLiveOwnerAdapter.h"
#include "MM6ChestSeedAndOwnership.h"
#include "MM6PersistentEventPublication.h"
#include "MM6PostRebuildPidSlots.h"
#include "MM6RawSessionStage.h"
#include "MM6SavedActorPidPolicy.h"

namespace mm6 {
namespace {

ChestRecordSet readChestRecords(std::span<const std::uint8_t> delta,
                                const MM6IndoorSnapshot *indoorBase) {
    ChestRecordSet result{};
    if (indoorBase) {
        const Blob source = Blob::copy(delta.data(), delta.size());
        const MM6IndoorSnapshot wire = deserializeMM6IndoorDelta(source, *indoorBase);
        const Blob &section = wire.section(MM6IndoorSectionKind::Chests).bytes;
        if (!copyChestRecords({static_cast<const std::uint8_t *>(section.data()),
                               section.size()}, &result))
            throw std::runtime_error("MM6 post-world indoor chest section is incomplete");
    } else {
        const OutdoorDeltaWire wire = decodeOutdoorDelta(delta);
        if (!copyChestRecords(wire.part(OutdoorDeltaPart::Chests), &result))
            throw std::runtime_error("MM6 post-world outdoor chest section is incomplete");
    }
    return result;
}

void requireFinalChests(const StagedRawSessionFields &staged,
                        const PostWorldOwnerView &live,
                        const MM6IndoorSnapshot *indoorBase, GameTitle title) {
    const ChestSaveFields &saved = staged.raw.saved.chest;
    if (!staged.world.chestTitle.ready || !live.chestTitle->ready ||
        captureChestSaveFields(staged.world.chestTitle, saved.activeDeltaName,
                               staged.world.rawActiveDelta, title) != saved)
        throw std::runtime_error("MM6 post-world source CHST owner diverged");

    const ChestRecordSet initialRecords = readChestRecords(saved.rawActiveDelta, indoorBase);
    std::vector<Chest> initialChests(20);
    for (int index = 0; index < 20; ++index) {
        const Chest_MM7 source = projectMM6ChestRecord(initialRecords[index]);
        reconstruct(source, &initialChests[index], tags::context<int>(index));
    }
    std::vector<std::uint8_t> initialDelta = saved.rawActiveDelta;
    ChestTitleState initialTitle = staged.world.chestTitle;
    ChestLiveBinding binding{title, live.levelType, &initialChests,
                             &initialDelta, &initialTitle, indoorBase};
    const auto partyOwned = scanPartyOwnedChestArtifacts(title, staged.party.party);
    if (!partyOwned)
        throw std::runtime_error("MM6 post-world Party artifact ownership is unavailable");
    const auto expected = stageChestLiveOwnerHandoff(
        binding, live.maps->pInfos[live.loadedMapId].mapTreasureLevel,
        *live.items, *partyOwned);
    if (!expected || live.decodedDelta.size() != expected->deltaBytes.size() ||
        !std::equal(live.decodedDelta.begin(), live.decodedDelta.end(),
                    expected->deltaBytes.begin()) ||
        live.chestTitle->lcgState != expected->nextLcgState ||
        live.chestTitle->generatedArtifactFlags != expected->generatedArtifactFlags ||
        live.chests.size() != expected->chests.size())
        throw std::runtime_error("MM6 post-world CHST generation differs from saved delta");

    const ChestRecordSet finalRecords = readChestRecords(live.decodedDelta, indoorBase);
    for (int index = 0; index < 20; ++index) {
        ChestRuntimeProjection projected{live.chests[index], finalRecords[index]};
        const auto replay = stageChestRuntimeRecord(title, projected);
        if (!replay || *replay != finalRecords[index])
            throw std::runtime_error("MM6 post-world chest collection differs from sidecar");
    }
}

} // namespace

void requireRawV2PostWorldOwnerAgreement(const StagedRawSessionFields &staged,
                                        const PostWorldOwnerView &live,
                                        GameTitle title) {
    if (title != GameTitle::MM6 || !live.maps || !live.items || !live.persistent ||
        !live.chestTitle || live.loadedMapId < MAP_FIRST || live.loadedMapId > MAP_LAST)
        throw std::runtime_error("MM6 post-world check requires explicit title-local owners");
    requireNoActiveSavedActorOverlays(staged.raw.saved.metadata.overlays, title);
    const std::string &loadedName = live.maps->pInfos[live.loadedMapId].fileName;
    if (loadedName.empty() || loadedName != staged.raw.saved.metadata.header.mapFileName)
        throw std::runtime_error("MM6 post-world map differs from saved HEAD");

    const MM6IndoorSnapshot *indoorBase = nullptr;
    std::string expectedDeltaName;
    if (const auto *active = std::get_if<StagedIndoorMap>(&staged.raw.activeMap)) {
        if (live.levelType != LEVEL_INDOOR || active->mapName != loadedName ||
            !loadedName.ends_with(".blv") || !staged.indoorPlan || staged.outdoorPlan ||
            staged.indoorPlan->mapName != loadedName)
            throw std::runtime_error("MM6 post-world indoor map plan differs from loaded map");
        expectedDeltaName = loadedName.substr(0, loadedName.size() - 4) + ".dlv";
        indoorBase = &active->base;
        requirePostRebuildPidSlotAgreement(
            *staged.indoorPlan, loadedName, live.actors, live.sprites, title);
    } else {
        const auto &outdoor = std::get<StagedOutdoorMap>(staged.raw.activeMap);
        if (live.levelType != LEVEL_OUTDOOR || outdoor.mapName != loadedName ||
            !loadedName.ends_with(".odm") || !staged.outdoorPlan || staged.indoorPlan ||
            staged.outdoorPlan->mapName != loadedName)
            throw std::runtime_error("MM6 post-world outdoor map plan differs from loaded map");
        expectedDeltaName = loadedName.substr(0, loadedName.size() - 4) + ".ddm";
        requirePostRebuildPidSlotAgreement(
            *staged.outdoorPlan, loadedName, live.actors, live.sprites, title);
    }
    if (staged.raw.saved.chest.activeDeltaName != expectedDeltaName)
        throw std::runtime_error("MM6 post-world CHST names another active delta");
    requirePostMapPersistentEvents(
        staged.persistentEvents, live.decodedDelta, *live.persistent, title);
    requireFinalChests(staged, live, indoorBase, title);
}

} // namespace mm6
