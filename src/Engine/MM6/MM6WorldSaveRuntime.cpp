#include "MM6WorldSaveRuntime.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/OutdoorTileProjection.h"
#include "Engine/Engine.h"
#include "Engine/Pid.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "MM6BuffSaveFields.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6DurableSaveFields.h"
#include "MM6HeldItemSaveFields.h"
#include "MM6HirelingSaveFields.h"
#include "MM6ResistanceSaveFields.h"
#include "MM6RecoverySaveFields.h"
#include "MM6RawSaveRuntime.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 world save fields cannot apply to another title");
}

std::string deltaCompanion(std::string_view mapName) {
    if (!mapName.ends_with(".blv") && !mapName.ends_with(".odm"))
        throw std::runtime_error("MM6 active map has no delta companion");
    return std::string(mapName.substr(0, mapName.size() - 4)) +
        (mapName.ends_with(".blv") ? ".dlv" : ".ddm");
}

std::vector<std::uint8_t> copiedBytes(const Blob &blob) {
    if (blob.empty())
        return {};
    const auto *start = static_cast<const std::uint8_t *>(blob.data());
    return {start, start + blob.size()};
}

StagedWorldFields stageWorldFromFields(const NPCStats &liveNpcs,
                                      const NpcSaveFields &npcs,
                                      const MapSaveFields &maps) {
    StagedWorldFields staged;
    staged.npcs = liveNpcs;
    staged.npcs.pNPCData = npcs.npcData;
    staged.npcs.pGroups = npcs.groups;
    staged.npcs.uNumNewNPCs = npcs.activeCount;
    for (const SavedMapDelta &delta : maps.deltas)
        staged.mapDeltas.emplace(delta.name, Blob::copy(delta.bytes.data(), delta.bytes.size()));
    return staged;
}

void validateSavedOverlayActorPids(const std::array<SaveOverlayField, 50> &overlays,
                                   const SavedActiveMapCounts &active) {
    for (const SaveOverlayField &overlay : overlays) {
        if (overlay.animationLength <= 0)
            continue;
        const Pid pid = Pid::fromPacked(overlay.packedPid);
        // The current overlay producer is Actor::AddOnDamageOverlay, which
        // supplies only actor PIDs. This checks the saved delta index, not
        // the actor identity after respawn, debug filters or load events.
        if (pid.type() != OBJECT_Actor || static_cast<std::size_t>(pid.id()) >= active.actors)
            throw std::runtime_error("MM6 active overlay references an absent saved actor");
    }
}

void requireNoUnrepresentedBeacons(const Party &party) {
    for (const Character &character : party.pCharacters)
        for (const auto &beacon : character.vBeacons)
            if (beacon)
                throw std::runtime_error("MM6 beacon map identity and image save fields are not represented");
}

bool hasItemPayload(const Item &item) {
    return item.itemId != ITEM_NULL || item.potionPower || item.goldAmount ||
           item.standardEnchantment || item.standardEnchantmentStrength ||
           item.specialEnchantment != ITEM_ENCHANTMENT_NULL || item.numCharges ||
           item.maxCharges || item.flags || item.lichJarCharacterIndex != -1 ||
           item.enchantmentExpirationTime.ticks();
}

void requireNoUnrepresentedCommerce(const Party &party) {
    // These are inherited MM7-indexed arrays, not an MM6 house-keyed owner.
    // Reject nonempty values until the original MM6 tuple and new storage are
    // proved; copying them would silently lose guild or bounty identity.
    for (HouseId id : party.standartItemsInShops.indices()) {
        if (party.PartyTimes.shopNextRefreshTime[id].ticks() ||
            party.PartyTimes.shopBanTimes[id].ticks() || party.InTheShopFlags[id])
            throw std::runtime_error("MM6 shop state lacks a title-specific save owner");
        for (const Item &item : party.standartItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 shop stock lacks a title-specific save owner");
        for (const Item &item : party.specialItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 special stock lacks a title-specific save owner");
    }
    for (HouseId id : party.spellBooksInGuilds.indices()) {
        if (party.PartyTimes.guildNextRefreshTime[id].ticks())
            throw std::runtime_error("MM6 guild refresh lacks a title-specific save owner");
        for (const Item &item : party.spellBooksInGuilds[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 guild stock lacks a title-specific save owner");
    }
    for (HouseId id : party.PartyTimes.bountyHuntNextGenTime.indices())
        if (party.PartyTimes.bountyHuntNextGenTime[id].ticks() ||
            party.monster_id_for_hunting[id] != MONSTER_INVALID ||
            party.monster_for_hunting_killed[id])
            throw std::runtime_error("MM6 bounty state lacks a title-specific save owner");
}

} // namespace

StagedKnownWorldAndVisuals stageKnownWorldAndVisuals(
    const NPCStats &liveNpcs, const Timer &liveTimer, const ActiveOverlayList &liveOverlays,
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    std::size_t overlayTableCount, GameTitle title) {
    requireMM6(title);
    ValidatedKnownMapSave validated = decodeKnownSaveWithActiveMapCounts(
        bytes, expectedNames, readBase, verifyItem);
    StagedWorldFields world = stageWorldFromFields(
        liveNpcs, validated.known.npcs, validated.known.maps);
    validateSavedOverlayActorPids(validated.known.metadata.overlays, validated.active);
    StagedSessionVisuals visuals = stageSessionVisuals(
        liveTimer, liveOverlays, {validated.known.metadata.timer, validated.known.metadata.overlays},
        overlayTableCount, title);
    ChestTitleState chestTitle;
    std::vector<std::uint8_t> rawActiveDelta;
    restoreChestFieldsBeforeMutation(validated.known.chest, &chestTitle, &rawActiveDelta, title);
    return {std::move(validated.known), std::move(world), std::move(visuals),
            std::move(validated.activeMap), chestTitle, std::move(rawActiveDelta)};
}

StagedRawWorldAndVisuals stageRawWorldAndVisuals(
    const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, const StagedRawTitleOwners &raw,
    std::size_t overlayTableCount, GameTitle title) {
    requireMM6(title);
    StagedWorldFields world = stageWorldFromFields(
        liveNpcs, raw.saved.npcs, raw.saved.maps);
    validateSavedOverlayActorPids(raw.saved.metadata.overlays, raw.activeCounts);
    StagedSessionVisuals visuals = stageSessionVisuals(
        liveTimer, liveOverlays,
        {raw.saved.metadata.timer, raw.saved.metadata.overlays},
        overlayTableCount, title);
    ChestTitleState chestTitle;
    std::vector<std::uint8_t> rawActiveDelta;
    restoreChestFieldsBeforeMutation(raw.saved.chest, &chestTitle, &rawActiveDelta, title);
    return {std::move(world), std::move(visuals), std::move(chestTitle),
            std::move(rawActiveDelta)};
}

StagedKnownSession stageKnownSession(
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::uint8_t> bytes,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount, GameTitle title) {
    StagedKnownWorldAndVisuals worldAndVisuals = stageKnownWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, bytes, expectedNames, readBase, verifyItem,
        overlayTableCount, title);

    // A partial format must not carry a stale live beacon into the restored
    // Party or silently omit an active beacon during capture.
    requireNoUnrepresentedBeacons(liveParty);
    requireNoUnrepresentedCommerce(liveParty);

    // Every adapter stages on its input copy. The core clock must precede
    // recovery, while all late ITEM/HELD/recovery checks precede any commit.
    Party staged = stageCoreParty(liveParty, bytes, title);
    staged = stageDurableParty(staged, bytes, title);
    staged = stageResistanceParty(staged, bytes, title);
    staged = stageBuffParty(staged, bytes, title);
    staged = stageHirelingParty(staged, bytes, title);
    staged = stageHeldItemParty(staged, bytes, title);
    staged = stageRecoveryParty(staged, bytes, title);
    ShopGuildClockFields clocks = captureShopGuildClocks(
        worldAndVisuals.known.shopGuildClocks, title);
    // Frame-derived half-speed remainder is not a save field. Map entry already
    // cancels delayed speech; clear the whole tuple here so a detached reentry
    // candidate never inherits it from the unrelated live Party.
    staged._roundingDt = 0_ticks;
    staged._delayedReactionTimer = 0_ticks;
    staged._delayedReactionSpeech = SPEECH_NONE;
    staged._delayedReactionCharacterId = -1;
    return {std::move(staged), std::move(worldAndVisuals), clocks};
}

namespace {

DetachedOutdoorWorldPlan prepareDetachedOutdoorWorldPlanFromOwners(
    const StagedActiveMap &activeMap, std::string_view savedMapName,
    std::string_view chestDeltaName, std::span<const std::uint8_t> rawActiveDelta,
    std::size_t tileTableCount, GameTitle title) {
    requireMM6(title);
    if (tileTableCount == 0 || tileTableCount > std::size_t(std::numeric_limits<std::int16_t>::max()) + 1)
        throw std::runtime_error("MM6 detached outdoor plan requires a bounded tile table");
    const auto *active = std::get_if<StagedOutdoorMap>(&activeMap);
    if (!active || active->mapName != savedMapName ||
        !active->mapName.ends_with(".odm") ||
        chestDeltaName !=
            active->mapName.substr(0, active->mapName.size() - 4) + ".ddm" ||
        active->base.bytes.empty())
        throw std::runtime_error("MM6 detached outdoor plan lacks matching active owners");

    DetachedOutdoorWorldPlan plan;
    plan.mapName = active->mapName;
    deserializeMM6Outdoor(Blob::view(active->base.bytes.data(), active->base.bytes.size()), &plan.base);
    plan.savedDelta = active->delta;
    plan.activeSidecar = decodeMM6OutdoorDelta(
        Blob::view(rawActiveDelta.data(), rawActiveDelta.size()));
    if (plan.base.models.size() != active->base.modelCount ||
        active->base.modelFaces.size() != plan.base.models.size() ||
        plan.base.modelExtras.size() != plan.base.models.size() ||
        plan.base.decorations.size() != active->base.decorationCount ||
        plan.base.decorationPidList.size() != active->base.decorationPidCount ||
        plan.base.spawnPoints.size() != active->base.spawnCount ||
        plan.savedDelta.actors.size() != plan.activeSidecar.actors.size() ||
        plan.savedDelta.spriteObjects.size() != plan.activeSidecar.spriteObjects.size() ||
        plan.savedDelta.chests.size() != plan.activeSidecar.chests.size())
        throw std::runtime_error("MM6 detached outdoor plan has mismatched base or sidecar counts");

    std::size_t vertexOffset = 0;
    for (std::size_t modelIndex = 0; modelIndex < plan.base.models.size(); ++modelIndex) {
        const BSPModelData_MM7 &model = plan.base.models[modelIndex];
        const BSPModelExtras_MM7 &extras = plan.base.modelExtras[modelIndex];
        if (model.numVertices != extras.vertices.size() ||
            model.numFaces != extras.faces.size() ||
            extras.faces.size() != extras.faceTextures.size() ||
            model.numFaces != active->base.modelFaces[modelIndex].count ||
            extras.vertices.size() > std::size_t(std::numeric_limits<int>::max()) - vertexOffset)
            throw std::runtime_error("MM6 detached outdoor model shape is invalid");
        for (const Vec3i &vertex : extras.vertices)
            plan.vertices.push_back(vertex.toFloat());
        for (const ODMFace_MM7 &face : extras.faces) {
            if (face.numVertices > face.vertexIds.size())
                throw std::runtime_error("MM6 outdoor face exceeds its vertex slots");
            std::vector<std::size_t> refs;
            refs.reserve(face.numVertices);
            for (std::size_t i = 0; i < face.numVertices; ++i) {
                const int id = face.vertexIds[i];
                if (id < 0 || std::size_t(id) >= extras.vertices.size())
                    throw std::runtime_error("MM6 outdoor face references an absent model vertex");
                refs.push_back(vertexOffset + std::size_t(id));
            }
            plan.faceVertexIds.push_back(std::move(refs));
        }
        vertexOffset += extras.vertices.size();
    }
    for (const SpawnPoint_MM7 &spawn : plan.base.spawnPoints)
        if (spawn.type != std::to_underlying(OBJECT_Actor) &&
            spawn.type != std::to_underlying(OBJECT_Sprite))
            throw std::runtime_error("MM6 outdoor spawn has an unsupported runtime type");
    plan.spawnPoints.reserve(plan.base.spawnPoints.size());
    for (const SpawnPoint_MM7 &spawn : plan.base.spawnPoints) {
        SpawnPoint runtime;
        reconstruct(spawn, &runtime);
        plan.spawnPoints.push_back(runtime);
    }
    std::array<int, 4> tileBases;
    for (std::size_t i = 0; i < tileBases.size(); ++i) {
        const int baseId = plan.base.tileTypes[i].tileId;
        if (baseId == 0 || std::size_t(baseId) + 36 > tileTableCount)
            throw std::runtime_error("MM6 detached outdoor tile selector is invalid");
        tileBases[i] = baseId;
    }
    plan.terrainTileIds.reserve(127 * 127);
    for (int y = 0; y < 127; ++y) {
        for (int x = 0; x < 127; ++x) {
            const int tileId = outdoorGlobalTileId(tileBases, plan.base.tileMap[y * 128 + x]);
            if (std::size_t(tileId) >= tileTableCount)
                throw std::runtime_error("MM6 detached outdoor tile ID exceeds the loaded table");
            plan.terrainTileIds.push_back(static_cast<std::int16_t>(tileId));
        }
    }
    return plan;
}

} // namespace

DetachedOutdoorWorldPlan prepareDetachedOutdoorWorldPlan(
    const StagedKnownWorldAndVisuals &staged, std::size_t tileTableCount,
    GameTitle title) {
    if (staged.rawActiveDelta != staged.known.chest.rawActiveDelta)
        throw std::runtime_error("MM6 detached outdoor plan has a divergent chest sidecar");
    return prepareDetachedOutdoorWorldPlanFromOwners(
        staged.activeMap, staged.known.metadata.header.mapFileName,
        staged.known.chest.activeDeltaName, staged.rawActiveDelta, tileTableCount, title);
}

DetachedOutdoorWorldPlan prepareDetachedRawOutdoorWorldPlan(
    const StagedRawTitleOwners &raw, std::size_t tileTableCount, GameTitle title) {
    return prepareDetachedOutdoorWorldPlanFromOwners(
        raw.activeMap, raw.saved.metadata.header.mapFileName,
        raw.saved.chest.activeDeltaName, raw.saved.chest.rawActiveDelta,
        tileTableCount, title);
}

KnownSaveFields captureKnownShopGuildClockFields(const KnownSaveFields &seed,
                                                const ShopGuildClockFields &owner,
                                                GameTitle title) {
    requireMM6(title);
    KnownSaveFields candidate = seed;
    candidate.shopGuildClocks = captureShopGuildClocks(owner, title);
    (void) encodeKnownSave(candidate);
    return candidate;
}

KnownSaveFields captureKnownPartyFields(const KnownSaveFields &seed, const Party &party,
                                       GameTitle title) {
    requireMM6(title);
    requireNoUnrepresentedBeacons(party);
    requireNoUnrepresentedCommerce(party);
    KnownSaveFields candidate = seed;
    candidate.core = captureCoreFields(party, title);
    candidate.durable = captureDurableFields(party, title);
    candidate.resistances = captureResistanceFields(party, title);
    candidate.buffs = captureBuffFields(party, title);
    candidate.hirelings = captureHirelingFields(party, title);
    candidate.heldItem = captureHeldItem(party, title);
    candidate.recovery = captureRecoveryFields(party, title);
    candidate.metadata.header.playingTicks = candidate.core.party.playingTicks;
    candidate.metadata.ui.activeCharacter = candidate.core.party.activeCharacter;
    (void) encodeKnownSave(candidate);
    return candidate;
}

void validateOutdoorMapIdentity(MapId currentMapId, const MapTable &mapTable,
                                std::string_view savedMapName,
                                std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    if (currentMapId < MAP_FIRST || currentMapId > MAP_LAST ||
        !savedMapName.ends_with(".odm") || expectedNames.size() != 67)
        throw std::runtime_error("MM6 outdoor capture has no bounded current map identity");

    const std::string &currentName = mapTable.pInfos[currentMapId].fileName;
    if (currentName != savedMapName ||
        std::count(expectedNames.begin(), expectedNames.end(), deltaCompanion(savedMapName)) != 1)
        throw std::runtime_error("MM6 outdoor map disagrees with HEAD or the 67-map manifest");
    for (MapId id : mapTable.pInfos.indices())
        if (id != currentMapId && mapTable.pInfos[id].fileName == currentName)
            throw std::runtime_error("ambiguous MM6 outdoor map-table filename");
}

KnownSaveFields captureKnownWorldFields(const KnownSaveFields &seed, const NPCStats &npcs,
                                        const std::unordered_map<std::string, Blob> &priorDeltas,
                                        std::string_view currentDeltaName,
                                        std::span<const std::uint8_t> currentDelta,
                                        std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    if (currentDeltaName != deltaCompanion(seed.metadata.header.mapFileName) ||
        currentDelta.empty() || priorDeltas.size() != expectedNames.size())
        throw std::runtime_error("MM6 world capture lacks a complete current-map delta");

    KnownSaveFields candidate = seed;
    candidate.npcs = captureNpcFields(npcs, title);
    candidate.maps.deltas.clear();
    candidate.maps.deltas.reserve(expectedNames.size());
    for (const std::string &name : expectedNames) {
        auto prior = priorDeltas.find(name);
        if (prior == priorDeltas.end())
            throw std::runtime_error("MM6 world capture is missing a saved map delta");
        candidate.maps.deltas.push_back({name, name == currentDeltaName
            ? std::vector<std::uint8_t>(currentDelta.begin(), currentDelta.end())
            : copiedBytes(prior->second)});
    }
    validateExpectedMapNames(candidate.maps, expectedNames);
    (void) encodeKnownSave(candidate);
    return candidate;
}

StagedWorldFields stageKnownWorldFields(const NPCStats &liveNpcs,
                                       std::span<const std::uint8_t> bytes,
                                       std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    KnownSaveFields candidate = decodeKnownSave(bytes);
    validateExpectedMapNames(candidate.maps, expectedNames);
    return stageWorldFromFields(liveNpcs, candidate.npcs, candidate.maps);
}

KnownSaveFields captureKnownWorldFromActiveMap(
    const KnownSaveFields &seed, const NPCStats &npcs,
    const std::unordered_map<std::string, Blob> &priorDeltas,
    std::span<const std::string> expectedNames, GameTitle title) {
    requireMM6(title);
    const std::string &mapName = seed.metadata.header.mapFileName;
    Blob changed;
    if (uCurrentlyLoadedLevelType == LEVEL_INDOOR && mapName.ends_with(".blv") &&
        pIndoor && pIndoor->filename == mapName) {
        changed = encodeMM6IndoorChangedState(*pIndoor);
    } else if (uCurrentlyLoadedLevelType == LEVEL_OUTDOOR && mapName.ends_with(".odm") && pOutdoor) {
        if (!engine || engine->gameTitle() != GameTitle::MM6 || !pMapTable)
            throw std::runtime_error("MM6 outdoor capture requires its loaded engine map table");
        validateOutdoorMapIdentity(engine->_currentLoadedMapId, *pMapTable, mapName, expectedNames, title);
        changed = encodeMM6OutdoorChangedState(*pOutdoor);
    } else {
        throw std::runtime_error("MM6 active level and save map do not match");
    }
    if (changed.empty())
        throw std::runtime_error("MM6 changed-state writer returned an empty delta");
    KnownSaveFields candidate = captureKnownWorldFields(seed, npcs, priorDeltas, deltaCompanion(mapName),
                                   {static_cast<const std::uint8_t *>(changed.data()), changed.size()},
                                   expectedNames, title);
    const std::vector<std::uint8_t> &sidecar = mapName.ends_with(".blv") ?
        pIndoor->mm6DeltaBytes : pOutdoor->mm6DeltaBytes;
    candidate.chest = captureChestSaveFields(chestTitleState(), deltaCompanion(mapName), sidecar, title);
    (void) encodeKnownSave(candidate);
    return candidate;
}

} // namespace mm6
