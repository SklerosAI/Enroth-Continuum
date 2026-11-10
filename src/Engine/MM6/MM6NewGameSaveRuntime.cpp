#include "MM6NewGameSaveRuntime.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MM6AtomicSaveFile.h"
#include "MM6Archive.h"
#include "MM6BuffSaveFields.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6DurableSaveFields.h"
#include "MM6HeldItemSaveFields.h"
#include "MM6HirelingSaveFields.h"
#include "MM6NpcSaveFields.h"
#include "MM6RecoverySaveFields.h"
#include "MM6ResistanceSaveFields.h"

namespace mm6 {

KnownSaveFields captureFirstMapKnownSeed(
    const Party &party, const NPCStats &npcs, const Timer &timer,
    const ActiveOverlayList &overlays, std::string_view displayName,
    std::size_t overlayTableCount, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 first-map capture cannot read another title");

    Party placed = party;
    placed.pos = Vec3f(-10178, -11206, 340);
    placed.uFallStartZ = 340;
    placed._viewPitch = 0;
    placed._viewYaw = 128;

    KnownSaveFields fields;
    fields.core = captureCoreFields(placed, title);
    fields.durable = captureDurableFields(placed, title);
    fields.resistances = captureResistanceFields(placed, title);
    fields.buffs = captureBuffFields(placed, title);
    fields.hirelings = captureHirelingFields(placed, title);
    fields.heldItem = captureHeldItem(placed, title);
    fields.recovery = captureRecoveryFields(placed, title);
    fields.npcs = captureNpcFields(npcs, title);
    SessionVisualFields visuals = captureSessionVisualFields(
        timer, overlays, overlayTableCount, title);
    fields.metadata.header = {std::string(displayName), std::string(NEW_GAME_FIRST_MAP),
                              fields.core.party.playingTicks};
    fields.metadata.timer = visuals.timer;
    fields.metadata.overlays = visuals.overlays;
    fields.metadata.ui.activeCharacter = fields.core.party.activeCharacter;
    fields.chest.activeDeltaName = "oute3.ddm";
    fields.shopGuildClocks.ready = true;
    return fields;
}

std::vector<std::uint8_t> encodeOfflineNewGameAutosave(
    const KnownSaveFields &seed, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 new-game autosave cannot encode another title");
    if (seed.metadata.header.mapFileName != NEW_GAME_FIRST_MAP ||
        expectedNames.size() != 67 || !readDefaultDelta || !readBase || !verifyItem)
        throw std::runtime_error("MM6 new-game autosave lacks first-map or verified archive inputs");
    if (seed.core.party.position[2] != 340 || seed.core.party.fallStartZ != 340)
        throw std::runtime_error("MM6 new-game autosave lacks its first-map fall start");

    KnownSaveFields candidate = seed;
    candidate.maps.deltas.clear();
    candidate.maps.deltas.reserve(expectedNames.size());
    for (const std::string &name : expectedNames) {
        Blob entry = readDefaultDelta(name);
        if (entry.empty())
            throw std::runtime_error("MM6 new-game archive has an empty default map delta");
        const auto *start = static_cast<const std::uint8_t *>(entry.data());
        candidate.maps.deltas.push_back({name, std::vector<std::uint8_t>(start, start + entry.size())});
    }

    // The first-map sidecar is the decoded default DDM. The caller supplies
    // the title-local RNG/ledger seed; no inherited MM7 state is consulted.
    if (candidate.chest.activeDeltaName != "oute3.ddm")
        throw std::runtime_error("MM6 new-game autosave lacks its CHST first-map identity");
    const auto active = std::find_if(candidate.maps.deltas.begin(), candidate.maps.deltas.end(),
        [](const SavedMapDelta &delta) { return delta.name == "oute3.ddm"; });
    if (active == candidate.maps.deltas.end())
        throw std::runtime_error("MM6 new-game autosave lacks its first-map delta");
    Blob decoded = decodeMM6ArchiveEntry(Blob::view(active->bytes.data(), active->bytes.size()));
    const auto *start = static_cast<const std::uint8_t *>(decoded.data());
    candidate.chest.rawActiveDelta.assign(start, start + decoded.size());

    std::vector<std::uint8_t> encoded = encodeKnownSave(candidate);
    (void) decodeKnownSaveWithActiveMapCounts(encoded, expectedNames, readBase, verifyItem);
    return encoded;
}

std::vector<std::uint8_t> writeOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    std::span<const std::string> expectedNames, const DefaultDeltaReader &readDefaultDelta,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem, GameTitle title) {
    std::vector<std::uint8_t> encoded = encodeOfflineNewGameAutosave(
        seed, expectedNames, readDefaultDelta, readBase, verifyItem, title);
    writeAtomicSaveFile(profileRoot, NEW_GAME_AUTOSAVE_SLOT, encoded);
    return encoded;
}

PreparedNewGameSession prepareOfflineNewGameSession(
    const KnownSaveFields &seed, const Party &liveParty, const NPCStats &liveNpcs,
    const Timer &liveTimer, const ActiveOverlayList &liveOverlays,
    std::span<const std::string> expectedNames, const DefaultDeltaReader &readDefaultDelta,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    std::size_t overlayTableCount, const ChestSeedEntropy &chestEntropy, GameTitle title) {
    if (title != GameTitle::MM6 || seed.metadata.header.mapFileName != NEW_GAME_FIRST_MAP ||
        seed.chest.activeDeltaName != "oute3.ddm" || !seed.shopGuildClocks.ready ||
        std::any_of(seed.shopGuildClocks.deadlineWords.begin(), seed.shopGuildClocks.deadlineWords.end(),
                    [](std::uint64_t value) { return value != 0; }))
        throw std::runtime_error("MM6 first-map transaction requires fresh title-local state");
    const auto partyArtifacts = scanPartyOwnedChestArtifacts(title, liveParty);
    if (!partyArtifacts || std::any_of(partyArtifacts->begin(), partyArtifacts->end(),
                                       [](std::uint8_t flag) { return flag != 0; }))
        throw std::runtime_error("MM6 first-map transaction cannot reset owned artifact state");

    ChestTitleState titleChest;
    if (!initializePortNewGameChestTitleState(title, &titleChest, chestEntropy))
        throw std::runtime_error("MM6 first-map transaction lacks a fresh chest seed");
    KnownSaveFields fresh = seed;
    fresh.chest.lcgState = titleChest.lcgState;
    fresh.chest.generatedArtifactFlags = titleChest.generatedArtifactFlags;

    std::vector<std::uint8_t> bytes = encodeOfflineNewGameAutosave(
        fresh, expectedNames, readDefaultDelta, readBase, verifyItem, title);
    StagedKnownSession staged = stageKnownSession(
        liveParty, liveNpcs, liveTimer, liveOverlays, bytes, expectedNames,
        readBase, verifyItem, overlayTableCount, title);
    if (!staged.worldAndVisuals.chestTitle.ready ||
        staged.worldAndVisuals.chestTitle.lcgState != titleChest.lcgState ||
        staged.worldAndVisuals.chestTitle.generatedArtifactFlags != titleChest.generatedArtifactFlags ||
        staged.shopGuildClocks != seed.shopGuildClocks)
        throw std::runtime_error("MM6 first-map title owners changed during detached stage");
    return {std::move(bytes), std::move(staged)};
}

PreparedNewGameSession writeAndStageOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount,
    const ChestSeedEntropy &chestEntropy, GameTitle title) {
    PreparedNewGameSession prepared = prepareOfflineNewGameSession(
        seed, liveParty, liveNpcs, liveTimer, liveOverlays, expectedNames,
        readDefaultDelta, readBase, verifyItem, overlayTableCount, chestEntropy, title);
    writeAtomicSaveFile(profileRoot, NEW_GAME_AUTOSAVE_SLOT, prepared.bytes);
    return prepared;
}

namespace {

std::unique_ptr<PublishedFirstMapCache> publishPreparedFirstMap(
    const std::filesystem::path &profileRoot, PreparedNewGameSession prepared,
    std::unordered_map<std::string, Blob> *mapDeltas, ChestTitleState *titleChest) {
    static_assert(std::is_nothrow_swappable_v<std::unordered_map<std::string, Blob>>);
    static_assert(std::is_nothrow_swappable_v<ChestTitleState>);
    static_assert(std::is_nothrow_copy_assignable_v<ChestTitleState>);
    // Allocate and move the return object before the disk or process changes.
    auto result = std::unique_ptr<PublishedFirstMapCache>(
        new PublishedFirstMapCache{std::move(prepared), {}, *titleChest, true});
    writeAtomicSaveFile(profileRoot, NEW_GAME_AUTOSAVE_SLOT, result->prepared.bytes);

    result->priorMapDeltas.swap(*mapDeltas);
    mapDeltas->swap(result->prepared.staged.worldAndVisuals.world.mapDeltas);
    *titleChest = result->prepared.staged.worldAndVisuals.chestTitle;
    return result;
}

} // namespace

std::unique_ptr<PublishedFirstMapCache> writeAndPublishOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount,
    const ChestSeedEntropy &chestEntropy,
    std::unordered_map<std::string, Blob> *mapDeltas, ChestTitleState *titleChest,
    GameTitle title) {
    if (title != GameTitle::MM6 || !mapDeltas || !titleChest)
        throw std::runtime_error("MM6 first-map publication lacks title-local owners");
    PreparedNewGameSession prepared = prepareOfflineNewGameSession(
        seed, liveParty, liveNpcs, liveTimer, liveOverlays, expectedNames,
        readDefaultDelta, readBase, verifyItem, overlayTableCount, chestEntropy, title);
    return publishPreparedFirstMap(profileRoot, std::move(prepared), mapDeltas, titleChest);
}

bool rollbackPublishedFirstMapCache(PublishedFirstMapCache *published,
                                   std::unordered_map<std::string, Blob> *mapDeltas,
                                   ChestTitleState *titleChest, GameTitle title) noexcept {
    if (title != GameTitle::MM6 || !published || !published->active || !mapDeltas || !titleChest)
        return false;
    mapDeltas->swap(published->priorMapDeltas);
    std::swap(*titleChest, published->priorChest);
    published->active = false;
    return true;
}

std::unique_ptr<PublishedFirstMapCache> runScopedFirstMapEntryForTest(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount,
    const ChestSeedEntropy &chestEntropy,
    std::unordered_map<std::string, Blob> *mapDeltas, ChestTitleState *titleChest,
    const FirstMapEntryProbe &probe, GameTitle title) {
    if (title != GameTitle::MM6 || !mapDeltas || !titleChest || !probe)
        throw std::runtime_error("MM6 scoped first-map entry lacks title-local owners or probe");
    PreparedNewGameSession prepared = prepareOfflineNewGameSession(
        seed, liveParty, liveNpcs, liveTimer, liveOverlays, expectedNames,
        readDefaultDelta, readBase, verifyItem, overlayTableCount, chestEntropy, title);
    return publishPreparedFirstMapForTest(profileRoot, std::move(prepared),
                                          mapDeltas, titleChest, probe, title);
}

std::unique_ptr<PublishedFirstMapCache> publishPreparedFirstMapForTest(
    const std::filesystem::path &profileRoot, PreparedNewGameSession prepared,
    std::unordered_map<std::string, Blob> *mapDeltas, ChestTitleState *titleChest,
    const FirstMapEntryProbe &probe, GameTitle title) {
    if (title != GameTitle::MM6 || !mapDeltas || !titleChest || !probe)
        throw std::runtime_error("MM6 scoped first-map entry lacks title-local owners or probe");
    // The observer may perform additional detached first-map checks, but it
    // must finish before either the disk slot or process owners are published.
    // No caller-controlled code runs in the write-and-swap interval below.
    probe(prepared.staged.party, prepared.staged.worldAndVisuals.world.mapDeltas,
          prepared.staged.worldAndVisuals.chestTitle);
    const detail::SaveSlotRollbackSnapshot priorSlot =
        detail::snapshotSaveSlotForRollback(profileRoot, NEW_GAME_AUTOSAVE_SLOT);
    std::unique_ptr<PublishedFirstMapCache> published;
    try {
        published = publishPreparedFirstMap(
            profileRoot, std::move(prepared), mapDeltas, titleChest);
        return published;
    } catch (...) {
        if (published)
            (void) rollbackPublishedFirstMapCache(published.get(), mapDeltas, titleChest, title);
        try {
            detail::restoreSaveSlotForRollback(profileRoot, NEW_GAME_AUTOSAVE_SLOT, priorSlot);
        } catch (...) {
            throw std::runtime_error("MM6 first-map entry failed and disk rollback also failed");
        }
        throw;
    }
}

std::optional<StagedOfflineSaveSlot> stageOfflineSaveSlotForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const Party &freshParty, const NPCStats &freshNpcs, const Timer &freshTimer,
    const ActiveOverlayList &freshOverlays, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    std::size_t overlayTableCount, GameTitle title) {
    if (title != GameTitle::MM6 || !readBase || !verifyItem)
        throw std::runtime_error("MM6 offline slot stage requires title-specific readers");
    std::unique_ptr<StagedKnownSession> staged;
    auto selected = preflightRecoverableSaveFile(profileRoot, slotName,
        [&](std::span<const std::uint8_t> bytes) {
            auto candidate = std::make_unique<StagedKnownSession>(stageKnownSession(
                freshParty, freshNpcs, freshTimer, freshOverlays, bytes,
                expectedNames, readBase, verifyItem, overlayTableCount, title));
            staged = std::move(candidate);
        });
    if (!selected)
        return std::nullopt;
    if (!staged)
        throw std::runtime_error("MM6 save selected without a complete staged session");
    return StagedOfflineSaveSlot{std::move(selected->bytes), std::move(*staged),
                                 selected->needsRecovery};
}

} // namespace mm6
