#pragma once

#include <deque>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6V6IndoorSaveFields.h"

namespace mm6 {

// Read-only pre-event join against the real indoor loader's ordered owners.
// Native v6 slot anchors prove immutable identity and source order; they do
// not encode complete mutable runtime state, so this cannot admit gameplay.
void requireV6IndoorPreEventOwnerAgreement(
    const StagedV6IndoorSaveFields &staged, std::string_view loadedMapName,
    const std::deque<Actor> &liveActors,
    const std::vector<SpriteObject> &liveSprites,
    const std::vector<LevelDecoration> &liveDecorations, GameTitle title);

} // namespace mm6
