#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "MM6KnownSaveFields.h"
#include "MM6MapSaveValidation.h"

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Timer.h"
#include "Engine/Snapshots/CompositeSnapshots.h"

#include "Utility/Memory/Blob.h"

struct MapTable;

namespace mm6 {

struct StagedWorldFields {
    NPCStats npcs;
    std::unordered_map<std::string, Blob> mapDeltas;
};

struct StagedKnownWorldAndVisuals {
    KnownSaveFields known;
    StagedWorldFields world;
    StagedSessionVisuals visuals;
    // Decoded active base and changed state, owned independently of input bytes.
    // Live map reconstruction and global commit remain separate gates.
    StagedActiveMap activeMap;
    // Validated MM6 title-local chest state and raw active delta on copies;
    // no vChests or process owner is assigned.
    ChestTitleState chestTitle;
    std::vector<std::uint8_t> rawActiveDelta;
};

struct StagedKnownSession {
    Party party;
    StagedKnownWorldAndVisuals worldAndVisuals;
    ShopGuildClockFields shopGuildClocks;
};

struct DetachedOutdoorWorldPlan {
    std::string mapName;
    // An owned MM6 projection into the common base snapshot. This contains
    // terrain, model geometry, decorations and spawn records, but no textures,
    // renderer objects, listener or OutdoorLocation instance.
    OutdoorLocation_MM7 base;
    MM6OutdoorDeltaSnapshot savedDelta;
    MM6OutdoorDeltaSnapshot activeSidecar;
    // Face references flattened across models in source order. These are
    // validated global vertex indices for a later target-explicit builder.
    std::vector<std::vector<std::size_t>> faceVertexIds;
};

// Prepare only owned outdoor data from the already staged active MM6 map and
// CHST sidecar. Rejects invalid model/face/spawn topology before any live
// OutdoorLocation, asset, listener, renderer or global collection is touched.
DetachedOutdoorWorldPlan prepareDetachedOutdoorWorldPlan(
    const StagedKnownWorldAndVisuals &staged, GameTitle title);

// Validate the entire current known-section set and all 67 base/delta pairs,
// then stage map/NPC buffers, the typed active map and paused timer/overlays
// on detached copies. This is partial: Party, active-map reconstruction and
// PID rebinding remain outside the result, so it cannot be committed live.
StagedKnownWorldAndVisuals stageKnownWorldAndVisuals(
    const NPCStats &liveNpcs, const Timer &liveTimer, const ActiveOverlayList &liveOverlays,
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    std::size_t overlayTableCount, GameTitle title);

// Compose every currently represented Party/Character family on one detached
// Party after the known-section and 67-map checks. Nonempty inherited shop,
// guild, bounty or beacon state rejects rather than silently carrying through
// an unrepresented owner. The result cannot be committed to a live session.
StagedKnownSession stageKnownSession(
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::uint8_t> bytes,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount, GameTitle title);

// Merge the represented Party/Character families into a detached known-save
// candidate. Nonempty unrepresented commerce or beacon state rejects. The
// caller must supply complete MM6 world and metadata fields.
KnownSaveFields captureKnownPartyFields(const KnownSaveFields &seed, const Party &party,
                                       GameTitle title);

// Overlay the source-backed MM6 shop/guild refresh deadlines from a ready
// title-local owner. Stock, ban and bounty tuples remain outside this section.
KnownSaveFields captureKnownShopGuildClockFields(const KnownSaveFields &seed,
                                                const ShopGuildClockFields &owner,
                                                GameTitle title);

// The outdoor object has no filename. Check the current MM6 engine map ID
// against the loaded MM6 map table, HEAD and the private 67-name delta set.
void validateOutdoorMapIdentity(MapId currentMapId, const MapTable &mapTable,
                                std::string_view savedMapName,
                                std::span<const std::string> expectedNames, GameTitle title);

// Copy actual NPC/group and all 67 saved map buffers into a known-section
// candidate. The current map's bounded encoded delta must be supplied and
// replaces the stale copy in priorDeltas. No globals are changed.
KnownSaveFields captureKnownWorldFields(const KnownSaveFields &seed, const NPCStats &npcs,
                                        const std::unordered_map<std::string, Blob> &priorDeltas,
                                        std::string_view currentDeltaName,
                                        std::span<const std::uint8_t> currentDelta,
                                        std::span<const std::string> expectedNames, GameTitle title);

// Stage NPC records/groups and the complete MAPS set on detached copies.
// The caller must validate each delta against its MM6 base and item templates
// before committing this candidate to live globals.
StagedWorldFields stageKnownWorldFields(const NPCStats &liveNpcs,
                                       std::span<const std::uint8_t> bytes,
                                       std::span<const std::string> expectedNames, GameTitle title);

// Read the active MM6 indoor/outdoor runtime through its bounded changed-state
// writer, then overlay that encoded delta into a detached known-save candidate.
// No map, NPC, prior-delta or save global is assigned.
KnownSaveFields captureKnownWorldFromActiveMap(
    const KnownSaveFields &seed, const NPCStats &npcs,
    const std::unordered_map<std::string, Blob> &priorDeltas,
    std::span<const std::string> expectedNames, GameTitle title);

} // namespace mm6
