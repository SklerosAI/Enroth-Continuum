#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6KnownSaveFields.h"
#include "MM6MapSaveValidation.h"
#include "MM6ChestSeedAndOwnership.h"
#include "MM6WorldSaveRuntime.h"

namespace mm6 {

// Read a default .dlv/.ddm entry from the title-specific Games archive. The
// caller must verify the archive and supply the signed 67-name MM6 manifest.
using DefaultDeltaReader = std::function<Blob(std::string_view)>;

constexpr std::string_view NEW_GAME_AUTOSAVE_SLOT = "autosave.ec6";
constexpr std::string_view NEW_GAME_FIRST_MAP = "oute3.odm";

// Capture all represented fresh-session owners before the first map is
// loaded. Placement is applied to a Party copy, never the live Party.
KnownSaveFields captureFirstMapKnownSeed(
    const Party &party, const NPCStats &npcs, const Timer &timer,
    const ActiveOverlayList &overlays, std::string_view displayName,
    std::size_t overlayTableCount, GameTitle title);

// Build only the currently represented known-section candidate. The seed must
// already contain detached Party, NPC and visual capture plus an explicitly
// sourced MM6 chest LCG/ledger seed. This replaces MAPS with all 67 default
// archive deltas, derives the decoded CHST sidecar and validates every pair.
// It is not a complete save or permission to open live SaveLoad.
std::vector<std::uint8_t> encodeOfflineNewGameAutosave(
    const KnownSaveFields &seed, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, GameTitle title);

// Validate the complete known candidate before touching disk, then use the
// native atomic .ec6 writer in an explicit MM6 profile. The live new-game
// route must stay guarded until omitted owners and reload are proved.
std::vector<std::uint8_t> writeOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    std::span<const std::string> expectedNames, const DefaultDeltaReader &readDefaultDelta,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem, GameTitle title);

// Preflight the first-map candidate and every represented live owner on
// detached copies before any disk or process mutation. A fresh local chest
// seed is drawn exactly once into a detached title owner; the process owner
// remains untouched. Stock, bounty and beacons must be empty at this seam.
struct PreparedNewGameSession {
    std::vector<std::uint8_t> bytes;
    StagedKnownSession staged;
};

PreparedNewGameSession prepareOfflineNewGameSession(
    const KnownSaveFields &seed, const Party &liveParty, const NPCStats &liveNpcs,
    const Timer &liveTimer, const ActiveOverlayList &liveOverlays,
    std::span<const std::string> expectedNames, const DefaultDeltaReader &readDefaultDelta,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    std::size_t overlayTableCount, const ChestSeedEntropy &chestEntropy, GameTitle title);

// The write follows complete detached preflight. The returned map buffers can
// later seed pMapDeltas, but this function does not assign it or open SaveLoad.
PreparedNewGameSession writeAndStageOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount,
    const ChestSeedEntropy &chestEntropy, GameTitle title);

struct PublishedFirstMapCache {
    PreparedNewGameSession prepared;
    std::unordered_map<std::string, Blob> priorMapDeltas;
    ChestTitleState priorChest;
    bool active = true;
};

// Publish only the prepared default-delta cache and CHST title owner after a
// successful atomic write. Both swaps are no-throw; the returned prior values
// allow a later map-entry failure to restore these two owners on a separate
// rollback path. SaveLoad remains guarded pending full session commit proof.
std::unique_ptr<PublishedFirstMapCache> writeAndPublishOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    const Party &liveParty, const NPCStats &liveNpcs, const Timer &liveTimer,
    const ActiveOverlayList &liveOverlays, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, std::size_t overlayTableCount,
    const ChestSeedEntropy &chestEntropy,
    std::unordered_map<std::string, Blob> *mapDeltas, ChestTitleState *titleChest,
    GameTitle title);

// Restore the two published process owners if first-map entry fails before
// gameplay resumes. The prior autosave on disk is a separate recovery concern.
bool rollbackPublishedFirstMapCache(PublishedFirstMapCache *published,
                                   std::unordered_map<std::string, Blob> *mapDeltas,
                                   ChestTitleState *titleChest, GameTitle title) noexcept;

} // namespace mm6
