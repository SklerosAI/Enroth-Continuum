#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "MM6EventRuntimeSaveFields.h"
#include "MM6GameplayRngSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6IndoorFaceOverrides.h"
#include "MM6V6IndoorDecorationRuntime.h"
#include "MM6V6IndoorSpriteRuntime.h"
#include "MM6V6IndoorActorRuntime.h"
#include "MM6RawSessionStage.h"

namespace mm6 {

// V6 starts a separate indoor branch from v2. These three exact native slot
// anchors establish BLV/DLV provenance. IDRT and ISRT own mutable decoration
// and sprite state. Actor runtime ownership and a complete live replay
// transaction are still required before any v6 slot can enter gameplay.
struct IndoorNativeSlotAnchor {
    std::string mapName;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> records;

    bool operator==(const IndoorNativeSlotAnchor &) const = default;
};

struct V6IndoorSaveFields {
    RawSaveFields raw;
    GeneratedNpcSaveFields generatedNpcs;
    GameplayRandomState gameplayRng;
    EventRuntimeSaveFields eventRuntime;
    IndoorFaceOverrides faceOverrides;
    IndoorNativeSlotAnchor actors;
    IndoorNativeSlotAnchor sprites;
    IndoorNativeSlotAnchor decorations;
    IndoorDecorationRuntimeFields runtimeDecorations;
    SpriteRuntimeSaveFields runtimeSprites;
    ActorRuntimeSaveFields runtimeActors;
};

struct StagedV6IndoorSaveFields {
    V6IndoorSaveFields saved;
    StagedRawSessionFields represented;
    std::vector<LevelDecoration> decorationReplay;
    std::vector<SpriteObject> spriteReplay;
    std::vector<Actor> actorReplay;
    // Explicit admission gate: IART is detached only. Full actor publication,
    // post-event equality and other world owners remain unproved.
    bool canEnterGameLoop() const { return false; }
};

// Copy exact source records from a validated indoor v2 plan. No live state.
void captureV6IndoorNativeAnchors(const StagedRawSessionFields &represented,
                                  IndoorNativeSlotAnchor *actors,
                                  IndoorNativeSlotAnchor *sprites,
                                  IndoorNativeSlotAnchor *decorations);

std::vector<std::uint8_t> encodeV6IndoorSave(const V6IndoorSaveFields &fields,
                                             GameTitle title);
V6IndoorSaveFields decodeV6IndoorSave(std::span<const std::uint8_t> bytes,
                                     GameTitle title);

// All 67 native pairs and detached v2 owners are validated first. The caller
// supplies a separately observed pre-OnMapReload face baseline. IFOV staging
// and the active native slot joins complete before any live mutation.
StagedV6IndoorSaveFields stageV6IndoorSave(
    std::span<const std::uint8_t> bytes, const NpcStaticBaseline &freshNpcs,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    const DecorationTable &decorationTable,
    std::size_t overlayTableCount, std::size_t tileTableCount,
    const IndoorBaseFaceReader &readPreEventFaces, GameTitle title);

} // namespace mm6
