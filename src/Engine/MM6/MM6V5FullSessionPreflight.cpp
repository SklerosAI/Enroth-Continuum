#include "MM6V5FullSessionPreflight.h"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "MM6V4SaveFields.h"
#include "MM6V5SaveFields.h"

namespace mm6 {

std::unique_ptr<V5FullSessionPreflight> stageV5FullSessionPreflightOnHeap(
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
    const DecorationTable &decorationTable,
    GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v5 full-session preflight requires MM6 title");
    const auto saved = std::unique_ptr<V5SaveFields>(
        new V5SaveFields(decodeV5Save(bytes, title)));
    if (encodeV5Save(*saved, title) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 v5 container is not canonical");
    auto v4 = std::unique_ptr<V4FullSessionPreflight>(new V4FullSessionPreflight(
        stageV4FullSessionPreflight(
        encodeV4Save(saved->v4, title), freshNpcs, expectedNames, readBase,
        verifyItem, itemTable, overlayTableCount, tileTableCount,
        runtimeActorCap, selectedRngType, rebuiltDecorations, title)));
    const std::string &mapName = v4->v3.actorMap.activeMap.mapName;
    const auto &nativeDelta = v4->saved.v3.raw.chest.rawActiveDelta;
    if (mapName != "oute3.odm" || saved->actors.mapName != mapName ||
        saved->actors.actors.size() != v4->v3.actorMap.activeActors.size() ||
        saved->decorations.mapName != mapName ||
        saved->sprites.mapName != mapName ||
        saved->sprites.records.size() != v4->v3.actorMap.activeMap.delta.spriteObjects.size())
        throw std::runtime_error("MM6 v5 runtime owners lack exact first-map slot lists");
    requireActorRuntimeBaselineAgreement(
        saved->actors, mapName, v4->saved.v3.activeActorPayload, title);
    requireDecorationRuntimeBaseAgreement(
        saved->decorations, mapName, readBase(mapName),
        v4->activeEventRuntime, decorationTable, title);
    requireSpriteRuntimeNativeAgreement(
        saved->sprites, mapName, nativeDelta, title);
    auto result = std::make_unique<V5FullSessionPreflight>();
    result->v4 = std::move(*v4);
    result->actors = std::move(saved->actors);
    result->decorations = std::move(saved->decorations);
    result->sprites = std::move(saved->sprites);
    return result;
}

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
    const DecorationTable &decorationTable,
    GameTitle title) {
    auto staged = stageV5FullSessionPreflightOnHeap(
        bytes, freshNpcs, expectedNames, readBase, verifyItem, itemTable,
        overlayTableCount, tileTableCount, runtimeActorCap, selectedRngType,
        rebuiltDecorations, decorationTable, title);
    return std::move(*staged);
}

} // namespace mm6
