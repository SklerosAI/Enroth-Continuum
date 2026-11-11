#include "MM6SessionSave.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"

#include "MM6Archive.h"
#include "MM6BuffSaveFields.h"
#include "MM6ChestMapLoadRoute.h"
#include "MM6ChestSaveFields.h"
#include "MM6ChestTitleState.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6DurableSaveFields.h"
#include "MM6HirelingSaveRuntime.h"
#include "MM6LoadedMapSource.h"
#include "MM6MapTransition.h"
#include "MM6NpcSaveFields.h"
#include "MM6NpcStaticBaseline.h"
#include "MM6PersistentEventPublication.h"
#include "MM6RawCursorImagePlan.h"
#include "MM6RawCursorShopProcessOwner.h"
#include "MM6RawPartyInventoryStage.h"
#include "MM6RawV2Admission.h"
#include "MM6RecoverySaveRuntime.h"
#include "MM6ResistanceSaveFields.h"
#include "MM6SaveMetadata.h"
#include "MM6ShopTitleState.h"

namespace mm6 {

static constexpr std::size_t RAW_SECTIONS = 19;
static constexpr int MAX_ITEM_ID = 580;

static void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 session save requires the MM6 title");
}

static bool isMM6ItemTemplate(std::int32_t id) {
    return id >= 1 && id <= MAX_ITEM_ID && !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
}

static std::string deltaNameOf(std::string_view mapName) {
    return std::string(mapName.substr(0, mapName.size() - 3)) + (mapName.ends_with(".blv") ? "dlv" : "ddm");
}

static void requireKnownItem(const Item &item) {
    if (item.itemId != ITEM_NULL && !isMM6ItemTemplate(std::to_underlying(item.itemId)))
        throw std::runtime_error("MM6 session save holds an item outside the title table");
}

std::vector<std::uint8_t> encodeSessionSave(const SessionSaveFields &fields, GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(encodeRawSave(fields.raw, title));
    container.version = SESSION_SAVE_VERSION;
    container.sections.push_back(encodeParkedMapSection(fields.maps, title));
    return encodeSaveContainer(container);
}

SessionSaveFields decodeSessionSave(std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(bytes);
    if (container.version != SESSION_SAVE_VERSION)
        throw std::runtime_error("MM6 session save has another EC6 version");
    if (container.sections.size() != RAW_SECTIONS + 1)
        throw std::runtime_error("MM6 session save has a missing or extra section");
    SessionSaveFields result;
    result.maps = decodeParkedMapSection(container, title);
    std::erase_if(container.sections, [](const SaveSection &section) { return section.tag == std::array{'P', 'R', 'K', 'M'}; });
    container.version = 2;
    result.raw = decodeRawSave(encodeSaveContainer(container), title);
    return result;
}

std::unique_ptr<StagedSessionSave> stageSessionSave(std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    if (!engine || engine->gameTitle() != title || !pGames_LOD || !pMapTable || !pItemTable || !pOverlayTable ||
        !pTileTable || !engine->resources())
        throw std::runtime_error("MM6 session save needs a loaded MM6 runtime");
    SessionSaveFields fields = decodeSessionSave(bytes, title);

    const std::string &activeMap = fields.raw.metadata.header.mapFileName;
    if (!std::ranges::contains(fields.maps, activeMap, &ParkedMapRuntime::mapName))
        throw std::runtime_error("MM6 session save lacks the lists of its active map");
    for (const ParkedMapRuntime &map : fields.maps) {
        const MapId id = pMapTable->GetMapInfo(map.mapName);
        if (id == MAP_INVALID || isMapIndoor(title, id) != map.mapName.ends_with(".blv"))
            throw std::runtime_error("MM6 session save holds a map outside the title table");
        for (const Actor &actor : map.actors)
            std::ranges::for_each(actor.items, requireKnownItem);
        for (const SpriteObject &sprite : map.sprites)
            requireKnownItem(sprite.containing_item);
    }

    const LoadedMapSource maps(*pGames_LOD, *pMapTable, title);
    const NpcStaticBaseline freshNpcs = loadNpcStaticBaseline(*engine->resources(), title);
    auto result = std::make_unique<StagedSessionSave>(StagedSessionSave{
        stageRawSessionFields(encodeRawSave(fields.raw, title), freshNpcs, maps.expectedNames(),
                              [&maps](std::string_view name) { return maps.readBase(name); }, isMM6ItemTemplate,
                              *pItemTable, pOverlayTable->overlays.size(), pTileTable->tileCount(), title),
        std::move(fields.maps)});
    if (result->represented.needsRecovery)
        throw std::runtime_error("MM6 session save needs recovery");
    return result;
}

std::vector<std::uint8_t> captureSessionSave(const RawSaveFields &sessionTemplate, std::string_view displayName,
                                             GameTitle title) {
    requireMM6(title);
    if (!engine || engine->gameTitle() != title || !pGames_LOD || !pMapTable || !pItemTable || !pParty || !pNPCStats ||
        engine->_currentLoadedMapId == MAP_INVALID || engine->_pendingTransition)
        throw std::runtime_error("MM6 session save needs a settled MM6 map");
    if (pParty->pPickedItem.itemId != ITEM_NULL)
        throw std::runtime_error("MM6 session save refuses an item held on the cursor");
    requireRawV2RepresentableState(*pParty, {}, title);
    (void) stageRawCursorImagePlan(*pParty, rawHeldCursorState(title), *pItemTable, title);
    const LoadedMapSource maps(*pGames_LOD, *pMapTable, title);
    if (pMapDeltas.size() != maps.expectedNames().size())
        throw std::runtime_error("MM6 session save map cache shape changed");

    std::vector<SessionMap> session = captureSessionMaps();
    SessionSaveFields fields;
    fields.raw = sessionTemplate;
    fields.raw.core = captureCoreFields(*pParty, title);
    const DurableSaveFields durable = captureDurableFields(*pParty, title);
    fields.raw.bits = durable.bits;
    fields.raw.effects = durable.effects;
    fields.raw.inventory = captureRawPartyInventories({*pParty, fields.raw.inventory}, *pItemTable, title);
    fields.raw.metadata = captureMetadata(std::string(displayName), title);
    fields.raw.metadata.overlays = captureSessionVisualFields(*gameTimer, ActiveOverlayList(), pOverlayTable->overlays.size(), title).overlays; // Hit flashes over actors, a load starts without them.
    fields.raw.npcs = captureNpcFields(*pNPCStats, title);
    fields.raw.resistances = captureResistanceFields(*pParty, title);
    fields.raw.buffs = captureBuffFields(*pParty, title);
    fields.raw.hirelings = captureHirelingFields(*pParty, title);
    fields.raw.recovery = captureRecoveryFields(*pParty, title);
    const auto held = captureRawHeldCursor(rawHeldCursorState(title), title);
    if (!held)
        throw std::runtime_error("MM6 session save cursor owner is not representable");
    fields.raw.heldItem = *held;
    fields.raw.shops = shopTitleState();

    const std::string activeDelta = deltaNameOf(fields.raw.metadata.header.mapFileName);
    for (SavedMapDelta &entry : fields.raw.maps.deltas) {
        const auto map = std::ranges::find(session, entry.name, &SessionMap::deltaName);
        if (map != session.end()) {
            const auto *begin = static_cast<const std::uint8_t *>(map->delta.data());
            entry.bytes.assign(begin, begin + map->delta.size());
        } else {
            const auto cached = pMapDeltas.find(entry.name);
            if (cached == pMapDeltas.end())
                throw std::runtime_error("MM6 session save map cache lacks a delta");
            const auto *begin = static_cast<const std::uint8_t *>(cached->second.data());
            entry.bytes.assign(begin, begin + cached->second.size());
        }
        if (entry.name == activeDelta)
            fields.raw.chest = captureChestSaveFields(chestTitleState(), activeDelta, entry.bytes, title);
    }
    if (fields.raw.chest.activeDeltaName != activeDelta)
        throw std::runtime_error("MM6 session save has no delta for its active map");
    for (SessionMap &map : session)
        fields.maps.push_back(std::move(map.runtime));
    return encodeSessionSave(fields, title);
}

void publishSessionSave(StagedSessionSave *staged, GameTitle title) {
    requireMM6(title);
    if (!engine || engine->gameTitle() != title || !pMapTable || !pParty || !pNPCStats)
        throw std::runtime_error("MM6 session load needs a fresh MM6 session");
    StagedRawSessionFields &represented = staged->represented;
    const MapId activeMap = pMapTable->GetMapInfo(represented.raw.saved.metadata.header.mapFileName);
    *pParty = represented.party.party;
    *pNPCStats = represented.world.world.npcs;
    *gameTimer = represented.world.visuals.timer;
    *pActiveOverlayList = represented.world.visuals.overlays;
    pMapDeltas = std::move(represented.world.world.mapDeltas);
    chestTitleState() = represented.world.chestTitle;
    publishFreshChildPersistentEventZeros(&engine->_persistentVariables, represented.persistentEvents, title);
    publishRawCursorShopOwners(&rawHeldCursorState(title), &shopTitleState(), represented.raw.saved,
                               {std::move(represented.raw.cursor), std::move(represented.raw.shops)}, title);

    std::vector<SessionMap> maps;
    for (ParkedMapRuntime &runtime : staged->maps) {
        SessionMap map;
        map.deltaName = deltaNameOf(runtime.mapName);
        map.delta = decodeMM6ArchiveEntry(pMapDeltas.at(map.deltaName));
        map.runtime = std::move(runtime);
        maps.push_back(std::move(map));
    }
    restoreSessionMaps(std::move(maps));
    admitChestMapLoad(activeMap);
    engine->_pendingTransition = MapDestination(activeMap, MAP_START_POINT_PARTY);
    engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
}

} // namespace mm6
