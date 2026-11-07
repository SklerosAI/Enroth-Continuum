#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <string_view>
#include <vector>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"

#include "MM6GeneratedNpcSaveFields.h"
#include "MM6V3OutdoorActorMapAgreement.h"

namespace mm6 {

struct PreEventActorReplayPlan {
    std::deque<Actor> actors;
    std::array<NPCData, 100> generatedRows;
    GameplayRandomState gameplayRng;
    int generatedCount = 0;
};

/**
 * Checks that a staged outdoor actor map can be replayed: one named map, a non-empty ACTR list that re-encodes to
 * the saved payload, matching native DDM and sidecar actor counts, and encodable GNPC and RNGS sections.
 *
 * @param agreement                     Detached v3 actor map agreement.
 * @param title                         Must be MM6.
 */
void requirePreEventActorReplaySource(const V3OutdoorActorMapAgreement &agreement, GameTitle title);

/**
 * Detached final pre-event agreement. ACTR is authoritative for mutable actor fields, while the rebuilt native DDM
 * prefix must keep exact PID anchors. GNPC must cover every generated peasant in the complete ACTR list.
 *
 * @param savedActors                   Complete ACTR list.
 * @param savedGenerated                Saved GNPC rows.
 * @param savedRng                      Saved RNGS state.
 * @param rebuiltActors                 Actors the native map load just rebuilt.
 * @param freshNpcs                     Live NPC tables, with no generated rows yet.
 * @param loadedMapName                 File name of the loaded map.
 * @param nativeActorCount              Native DDM actor count.
 * @param runtimeActorCap               Configured actor cap.
 * @param selectedRngType               Engine RNG type in use.
 * @param title                         Must be MM6.
 * @return                              Staged actors and generated rows. Nothing live is changed.
 */
PreEventActorReplayPlan stagePreEventActorReplay(
    const std::vector<Actor> &savedActors,
    const GeneratedNpcSaveFields &savedGenerated,
    const GameplayRandomState &savedRng,
    const std::deque<Actor> &rebuiltActors,
    const NPCStats &freshNpcs,
    std::string_view loadedMapName,
    std::size_t nativeActorCount,
    std::size_t runtimeActorCap,
    RandomEngineType selectedRngType,
    GameTitle title);

/**
 * Swaps staged actors and generated NPC rows into the live owners. RNGS is not published here, it is restored after
 * chest, arena, decoration and cursor setup, immediately before map events.
 *
 * @param plan                          Plan from `stagePreEventActorReplay`.
 * @param liveActors                    Live actor list.
 * @param liveNpcs                      Live NPC tables.
 * @param title                         Must be MM6.
 */
void publishPreEventActorReplay(PreEventActorReplayPlan plan, std::deque<Actor> *liveActors,
                                NPCStats *liveNpcs, GameTitle title);

} // namespace mm6
