#pragma once

#include <span>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6IndoorWorldPlan.h"
#include "MM6WorldSaveRuntime.h"

namespace mm6 {

// Read-only refusal gate after DoPrepareWorld(true), using the final arrays.
// It checks only stable source-backed slot anchors; matching anchors do not
// prove saved-delta provenance or permit active VISU overlays.
void requirePostRebuildPidSlotAgreement(
    const DetachedIndoorWorldPlan &plan, std::string_view loadedMapName,
    std::span<const Actor> actors, std::span<const SpriteObject> sprites,
    GameTitle title);

void requirePostRebuildPidSlotAgreement(
    const DetachedOutdoorWorldPlan &plan, std::string_view loadedMapName,
    std::span<const Actor> actors, std::span<const SpriteObject> sprites,
    GameTitle title);

} // namespace mm6
