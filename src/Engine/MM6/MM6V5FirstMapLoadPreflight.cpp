#include "MM6V5FirstMapLoadPreflight.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6LoadedMapSource.h"
#include "MM6NpcStaticBaseline.h"
#include "MM6SaveMetadata.h"
#include "MM6V5FullSessionPreflight.h"

namespace mm6 {
namespace {

constexpr std::string_view FIRST_MAP = "oute3.odm";

std::vector<LevelDecoration> sourceDecorations(const LoadedMapSource &maps) {
    const Blob baseBytes = maps.readBase(FIRST_MAP);
    auto source = std::make_unique<OutdoorLocation_MM7>();
    deserializeMM6Outdoor(baseBytes, source.get());
    std::vector<LevelDecoration> result(source->decorations.size());
    if (source->decorationNames.size() != result.size())
        throw std::runtime_error("MM6 v5 first-map base decorations lack source names");
    for (std::size_t i = 0; i < result.size(); ++i) {
        reconstruct(source->decorations[i], &result[i]);
        const auto &name = source->decorationNames[i];
        const auto end = std::find(name.begin(), name.end(), '\0');
        result[i].uDecorationDescID = pDecorationTable->decorationId(
            std::string(name.begin(), end));
        if (result[i].uDecorationDescID == DECORATION_NULL)
            throw std::runtime_error("MM6 v5 first-map decoration lacks a template");
    }
    return result;
}

} // namespace

std::unique_ptr<V5FullSessionPreflight> stageV5FirstMapLoadPreflight(
    std::span<const std::uint8_t> bytes, GameTitle title) {
    if (title != GameTitle::MM6 || !engine || engine->gameTitle() != title ||
        !engine->resources() || !engine->config || !pGames_LOD || !pMapTable ||
        !pItemTable || !pOverlayTable || !pTileTable || !pDecorationTable ||
        !application)
        throw std::runtime_error("MM6 v5 first-map load lacks title resources");
    const SaveContainer container = decodeSaveContainer(bytes);
    if (container.version != 5 ||
        decodeMetadataSections(container).header.mapFileName != FIRST_MAP)
        throw std::runtime_error("MM6 v5 first-map load has another HEAD map");

    const LoadedMapSource maps(*pGames_LOD, *pMapTable, title);
    const NpcStaticBaseline freshNpcs =
        loadNpcStaticBaseline(*engine->resources(), title);
    const auto verifyItem = [](std::int32_t id) {
        return id >= 1 && id <= 580 &&
            !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
    };
    auto rebuiltDecorations = sourceDecorations(maps);
    auto staged = stageV5FullSessionPreflightOnHeap(
        bytes, freshNpcs, maps.expectedNames(),
        [&maps](std::string_view name) { return maps.readBase(name); },
        verifyItem, *pItemTable, pOverlayTable->overlays.size(),
        pTileTable->tileCount(), engine->config->gameplay.MaxActors.value(),
        application->component<EngineRandomComponent>()->type(),
        rebuiltDecorations, *pDecorationTable, title);
    if (staged->v4.v3.actorMap.activeMap.mapName != FIRST_MAP ||
        staged->v4.saved.v3.raw.metadata.header.mapFileName != FIRST_MAP ||
        !staged->v4.v3.represented.outdoorPlan ||
        staged->v4.v3.represented.outdoorPlan->mapName != FIRST_MAP)
        throw std::runtime_error("MM6 v5 first-map load lacks exact active owners");
    return staged;
}

} // namespace mm6
