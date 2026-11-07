#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "MM6V5ActorRuntimeFields.h"

namespace mm6 {

struct IndoorNativeSlotAnchor;

ActorRuntimeSaveFields captureIndoorActorRuntime(
    std::string_view mapName, const IndoorNativeSlotAnchor &source,
    std::span<const Actor> live, GameTitle title);
void requireIndoorActorRuntimeSourceAgreement(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title);
std::vector<Actor> stageIndoorActorRuntime(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title);
void applyIndoorActorRuntimeToDetachedList(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, std::vector<Actor> *rebuilt,
    GameTitle title);

} // namespace mm6
