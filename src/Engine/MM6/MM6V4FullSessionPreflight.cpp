#include "MM6V4FullSessionPreflight.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mm6 {

V4FullSessionPreflight stageV4FullSessionPreflight(
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
    GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v4 full-session preflight requires MM6 title");
    V4SaveFields saved = decodeV4Save(bytes, title);
    const std::vector<std::uint8_t> v3Bytes = encodeV3Save(saved.v3, title);
    V3FullSessionPreflight v3 = stageV3FullSessionPreflight(
        v3Bytes, freshNpcs, expectedNames, readBase, verifyItem, itemTable,
        overlayTableCount, tileTableCount, runtimeActorCap, selectedRngType,
        title);
    if (!v3.represented.outdoorPlan ||
        v3.actorMap.activeMap.mapName != saved.v3.raw.metadata.header.mapFileName ||
        v3.actorMap.activeMap.base.decorationCount != rebuiltDecorations.size() ||
        v3.represented.outdoorPlan->mapName != v3.actorMap.activeMap.mapName ||
        v3.actorMap.activeSidecar.lastRespawnDay == 0)
        throw std::runtime_error("MM6 v4 active map lacks a matching decoration owner");

    const EventRuntimeMapEntry &active = requireEventRuntimeMapEntry(
        saved.eventRuntime, v3.actorMap.activeMap.mapName,
        v3.actorMap.activeSidecar.respawnCount,
        v3.actorMap.activeSidecar.lastRespawnDay, rebuiltDecorations, title);
    // V3 already binds HEAD, MAPS and CHST to the same native active delta.
    // EVRT is separate from that delta's opaque 200 persistent-event bytes.
    EventRuntimeMapEntry activeCopy = active;
    return {std::move(saved), std::move(v3), std::move(activeCopy)};
}

} // namespace mm6
