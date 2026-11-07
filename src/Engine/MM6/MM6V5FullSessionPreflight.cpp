#include "MM6V5FullSessionPreflight.h"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "MM6V4SaveFields.h"
#include "MM6V5SaveFields.h"

namespace mm6 {

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
    GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v5 full-session preflight requires MM6 title");
    auto saved = std::make_unique<V5SaveFields>(decodeV5Save(bytes, title));
    if (encodeV5Save(*saved, title) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 v5 container is not canonical");
    auto v4 = std::make_unique<V4FullSessionPreflight>(stageV4FullSessionPreflight(
        encodeV4Save(saved->v4, title), freshNpcs, expectedNames, readBase,
        verifyItem, itemTable, overlayTableCount, tileTableCount,
        runtimeActorCap, selectedRngType, rebuiltDecorations, title));
    const std::string &mapName = v4->v3.actorMap.activeMap.mapName;
    const auto &nativeDelta = v4->saved.v3.raw.chest.rawActiveDelta;
    if (mapName != "oute3.odm" || saved->sprites.mapName != mapName ||
        saved->sprites.records.size() != v4->v3.actorMap.activeMap.delta.spriteObjects.size())
        throw std::runtime_error("MM6 v5 SPRT lacks an exact first-map native slot list");
    requireSpriteRuntimeNativeAgreement(
        saved->sprites, mapName, nativeDelta, title);
    return {std::move(*v4), std::move(saved->sprites)};
}

} // namespace mm6
