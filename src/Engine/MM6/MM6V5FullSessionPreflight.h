#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "MM6V4FullSessionPreflight.h"
#include "MM6V5ActorRuntimeFields.h"
#include "MM6V5SpriteRuntimeFields.h"

namespace mm6 {

// Detached v5 outdoor candidate. V4 proves the Party, map, actor, NPC, RNG,
// event and chest owners. SPRT then binds every ordered runtime sprite to the
// same retained native DDM, without projecting its float coordinates into it.
struct V5FullSessionPreflight {
    V4FullSessionPreflight v4;
    ActorRuntimeSaveFields actors;
    SpriteRuntimeSaveFields sprites;
};

V5FullSessionPreflight stageV5FullSessionPreflight(
    std::span<const std::uint8_t> bytes,
    const NpcStaticBaseline &freshNpcs,
    std::span<const std::string> expectedNames,
    const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem,
    const ItemTable &itemTable,
    std::size_t overlayTableCount,
    std::size_t tileTableCount,
    std::size_t runtimeActorCap,
    RandomEngineType selectedRngType,
    std::span<LevelDecoration> rebuiltDecorations,
    GameTitle title);

} // namespace mm6
