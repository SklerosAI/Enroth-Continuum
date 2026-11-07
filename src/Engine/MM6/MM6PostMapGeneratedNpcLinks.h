#pragma once

#include <cstddef>
#include <deque>
#include <span>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"

namespace mm6 {

// Reject an MM6 map whose rebuilt actors would overrun the fixed generated
// NPC pool. Call before _461103_load_level_sub writes its first generated row.
size_t requirePreMapGeneratedNpcCapacity(const std::deque<Actor> &actors,
                                         const NPCStats &npcs, GameTitle title);

// Read the final actors and generated NPC pool after DoPrepareWorld(true).
// Proves only the bounded actor-to-row link shape produced by
// Engine::_461103_load_level_sub; no saved identity or grng state exists in EC6.
size_t requirePostMapGeneratedNpcLinks(std::span<const Actor> actors,
                                       const NPCStats &npcs, GameTitle title);

} // namespace mm6
