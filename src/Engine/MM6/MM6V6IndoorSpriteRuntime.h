#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "MM6V5SpriteRuntimeFields.h"

namespace mm6 {

struct IndoorNativeSlotAnchor;

SpriteRuntimeSaveFields captureIndoorSpriteRuntime(
    std::string_view mapName, const IndoorNativeSlotAnchor &source,
    std::span<const SpriteObject> live, GameTitle title);
void requireIndoorSpriteRuntimeSourceAgreement(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title);
std::vector<SpriteObject> stageIndoorSpriteRuntime(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title);
void applyIndoorSpriteRuntimeAfterMapReload(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, std::vector<SpriteObject> *afterEvent,
    GameTitle title);

} // namespace mm6
