#pragma once

#include <cstddef>
#include <span>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"

namespace mm6 {

// Read the final actors and generated NPC pool after DoPrepareWorld(true).
// Proves only the bounded actor-to-row link shape produced by
// Engine::_461103_load_level_sub; no saved identity or grng state exists in EC6.
size_t requirePostMapGeneratedNpcLinks(std::span<const Actor> actors,
                                       const NPCStats &npcs, GameTitle title);

} // namespace mm6
