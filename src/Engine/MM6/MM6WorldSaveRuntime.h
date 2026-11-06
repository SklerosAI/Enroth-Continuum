#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "MM6KnownSaveFields.h"

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "Utility/Memory/Blob.h"

struct MapTable;

namespace mm6 {

struct StagedWorldFields {
    NPCStats npcs;
    std::unordered_map<std::string, Blob> mapDeltas;
};

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
