#pragma once

#include <cstddef>
#include <deque>
#include <string_view>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"

#include "MM6GeneratedNpcSaveFields.h"

namespace mm6 {

struct PreEventGeneratedNpcReplay {
    GeneratedNpcSaveFields generated;
    GameplayRandomState gameplayRng;
    size_t generatedCount = 0;
};

// Detached admission at the narrow source-order boundary after map actor
// reconstruction and before _461103_load_level_sub() generates NPCs or
// onMapLoad() runs events. This does not publish actors, rows or RNG state.
PreEventGeneratedNpcReplay stagePreEventGeneratedNpcReplay(
    const GeneratedNpcSaveFields &savedGenerated,
    const GameplayRandomState &savedRng,
    const std::deque<Actor> &rebuiltActors,
    const NPCStats &freshNpcs,
    std::string_view loadedMapName,
    RandomEngineType selectedRngType,
    GameTitle title);

} // namespace mm6
