#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ActiveActorOutdoorDeltaCapture.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6V5DerivedActorNames.h"
#include "Engine/MM6/MM6GameplayRngSaveFields.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6NpcSaveFields.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
#include "Engine/MM6/MM6RawV2Admission.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6V4FullSessionPreflight.h"
#include "Engine/MM6/MM6V4SaveFields.h"
#include "Engine/MM6/MM6V5FullSessionPreflight.h"
#include "Engine/MM6/MM6V5OutdoorDecorationSource.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"
#include "Library/Platform/Application/PlatformApplication.h"

namespace {

constexpr std::string_view TARGET_MAP = "oute2.odm";
constexpr std::string_view TARGET_DELTA = "oute2.ddm";
constexpr std::string_view TARGET_SLOT = "candidate-v5-oute2.ec6";

std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 later outdoor needs an absolute disposable profile");
    const auto root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 later outdoor needs an owner-only system-temp profile");
    return root;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    if (std::filesystem::file_size(path) > 64 * 1024 * 1024)
        throw std::runtime_error("MM6 later outdoor slot exceeds EC6 bound");
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<LevelDecoration> sourceDecorations(const mm6::LoadedMapSource &maps,
                                               std::string_view mapName) {
    auto base = std::make_unique<OutdoorLocation_MM7>();
    deserializeMM6Outdoor(maps.readBase(mapName), base.get());
    if (base->decorations.size() != base->decorationNames.size())
        throw std::runtime_error("MM6 later outdoor source decoration names differ");
    std::vector<LevelDecoration> result(base->decorations.size());
    for (std::size_t slot = 0; slot < result.size(); ++slot) {
        reconstruct(base->decorations[slot], &result[slot]);
        const auto &name = base->decorationNames[slot];
        const auto end = std::find(name.begin(), name.end(), '\0');
        result[slot].uDecorationDescID = mm6::requireMM6OutdoorSourceDecorationId(
            base->decorations[slot].uDecorationDescID,
            std::string_view(name.data(), end - name.begin()), *pDecorationTable);
    }
    return result;
}

std::vector<std::uint8_t> captureLaterOutdoor(std::span<const std::uint8_t> sourceV4) {
    if (engine->gameTitle() != GameTitle::MM6 ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo(TARGET_MAP) ||
        engine->_pendingTransition || pPartyActionQueue->uNumActions)
        throw std::runtime_error("MM6 v5 later outdoor capture lacks settled map owners");
    auto saved = std::unique_ptr<mm6::V4SaveFields>(
        new mm6::V4SaveFields(mm6::decodeV4Save(sourceV4, GameTitle::MM6)));
    if (saved->v3.raw.metadata.header.mapFileName != "oute3.odm" ||
        saved->eventRuntime.maps.size() != 1 ||
        saved->eventRuntime.maps[0].mapName != "oute3.odm")
        throw std::runtime_error("MM6 later outdoor capture lacks strict first-map source");
    mm6::requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
    (void) mm6::stageRawCursorImagePlan(
        *pParty, mm6::rawHeldCursorState(GameTitle::MM6), *pItemTable, GameTitle::MM6);
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    if (pMapDeltas.size() != maps.expectedNames().size())
        throw std::runtime_error("MM6 later outdoor map cache count differs");

    mm6::V3SaveFields &fields = saved->v3;
    fields.raw.core = mm6::captureCoreFields(*pParty, GameTitle::MM6);
    const auto durable = mm6::captureDurableFields(*pParty, GameTitle::MM6);
    fields.raw.bits = durable.bits;
    fields.raw.effects = durable.effects;
    fields.raw.inventory = mm6::captureRawPartyInventories(
        {*pParty, fields.raw.inventory}, *pItemTable, GameTitle::MM6);
    fields.raw.metadata = mm6::captureMetadata(
        fields.raw.metadata.header.displayName, GameTitle::MM6);
    fields.raw.npcs = mm6::captureNpcFields(*pNPCStats, GameTitle::MM6);
    fields.raw.resistances = mm6::captureResistanceFields(*pParty, GameTitle::MM6);
    fields.raw.buffs = mm6::captureBuffFields(*pParty, GameTitle::MM6);
    fields.raw.hirelings = mm6::captureHirelingFields(*pParty, GameTitle::MM6);
    fields.raw.recovery = mm6::captureRecoveryFields(*pParty, GameTitle::MM6);
    const auto held = mm6::captureRawHeldCursor(
        mm6::rawHeldCursorState(GameTitle::MM6), GameTitle::MM6);
    if (!held)
        throw std::runtime_error("MM6 later outdoor cursor cannot be captured");
    fields.raw.heldItem = *held;
    fields.raw.shops = mm6::shopTitleState();

    const Blob delta = mm6::captureOutdoorDeltaWithNativeSpriteAnchors(
        *pOutdoor, TARGET_MAP, GameTitle::MM6);
    const auto *begin = static_cast<const std::uint8_t *>(delta.data());
    const std::vector<std::uint8_t> current(begin, begin + delta.size());
    fields.raw.chest = mm6::captureChestSaveFields(
        mm6::chestTitleState(), std::string(TARGET_DELTA), current, GameTitle::MM6);
    for (auto &entry : fields.raw.maps.deltas) {
        const auto it = pMapDeltas.find(entry.name);
        if (it == pMapDeltas.end())
            throw std::runtime_error("MM6 later outdoor map cache lacks source entry");
        const auto *cached = static_cast<const std::uint8_t *>(it->second.data());
        const std::vector<std::uint8_t> bytes(cached, cached + it->second.size());
        if (entry.name == TARGET_DELTA)
            entry.bytes = current;
        else if (entry.bytes != bytes)
            throw std::runtime_error("MM6 later outdoor nonactive map cache changed");
    }
    const std::vector<Actor> liveActors(pActors.begin(), pActors.end());
    const auto actors = mm6::canonicalActorsWithDerivedNamesRemoved(
        liveActors, *pMonsterStats, GameTitle::MM6);
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    fields.generatedNpcs = mm6::captureGeneratedNpcFields(
        pActors, *pNPCStats, TARGET_MAP, GameTitle::MM6);
    const auto random = application->component<EngineRandomComponent>();
    fields.gameplayRng = mm6::captureGameplayRngFields(*random, GameTitle::MM6);
    mm6::EventRuntimeMapEntry event;
    event.mapName = TARGET_MAP;
    event.respawnCount = pOutdoor->ddm.respawnCount;
    event.lastRespawnDay = pOutdoor->ddm.lastRespawnDay;
    event.mapVars = engine->_persistentVariables.mapVars;
    event.decorVars = engine->_persistentVariables.decorVars;
    auto baseDecorations = sourceDecorations(maps, TARGET_MAP);
    event.bindings = mm6::captureEventRuntimeDecorationBindings(
        baseDecorations, GameTitle::MM6);
    saved->eventRuntime.maps = {event};

    auto sprites = mm6::captureSpriteRuntimeFields(
        TARGET_MAP, current, pSpriteObjects, GameTitle::MM6);
    auto actorRuntime = mm6::captureActorRuntimeFields(
        TARGET_MAP, fields.activeActorPayload, actors, GameTitle::MM6);
    auto decorations = mm6::captureDecorationRuntimeFields(
        TARGET_MAP, maps.readBase(TARGET_MAP), event,
        *pDecorationTable, pLevelDecorations, GameTitle::MM6);
    auto complete = std::make_unique<mm6::V5SaveFields>(
        mm6::V5SaveFields{std::move(*saved), std::move(sprites),
                          std::move(actorRuntime), std::move(decorations)});
    return mm6::encodeV5Save(*complete, GameTitle::MM6);
}

void requireLaterOutdoorCandidate(std::span<const std::uint8_t> bytes,
                                  std::span<const std::uint8_t> sourceV4) {
    if (captureLaterOutdoor(sourceV4) !=
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 later outdoor live owners changed before atomic write");
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const mm6::NpcStaticBaseline freshNpcs =
        mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
    const auto verifyItem = [](std::int32_t id) {
        return id >= 1 && id <= 580 &&
            !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
    };
    auto rebuilt = sourceDecorations(maps, TARGET_MAP);
    auto stage = mm6::stageV5FullSessionPreflightOnHeap(
        bytes, freshNpcs, maps.expectedNames(),
        [&maps](std::string_view name) { return maps.readBase(name); },
        verifyItem, *pItemTable, pOverlayTable->overlays.size(),
        pTileTable->tileCount(), pActors.size(),
        application->component<EngineRandomComponent>()->type(),
        rebuilt, *pDecorationTable, GameTitle::MM6);
    if (stage->v4.v3.actorMap.activeMap.mapName != TARGET_MAP ||
        stage->v4.saved.v3.raw.metadata.header.mapFileName != TARGET_MAP ||
        stage->v4.saved.v3.raw.chest.activeDeltaName != TARGET_DELTA)
        throw std::runtime_error("MM6 later outdoor candidate lacks exact map join");
    const auto nativeEvents = mm6::capturePersistentEventBoundary(
        stage->v4.v3.represented.persistentEvents, GameTitle::MM6);
    const auto liveWire = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
    if (!std::ranges::equal(nativeEvents,
                            liveWire.part(mm6::OutdoorDeltaPart::PersistentBytes)) ||
        stage->v4.activeEventRuntime.mapVars != engine->_persistentVariables.mapVars ||
        stage->v4.activeEventRuntime.decorVars != engine->_persistentVariables.decorVars)
        throw std::runtime_error("MM6 later outdoor event owners changed");
}

} // namespace

GAME_TEST(MM6V5LaterOutdoorHeadless, DISABLED_WritesDistinctSecondOutdoorCandidate) {
    const char *enable = std::getenv("ENROTH_MM6_V5_OUTE2_WRITE_ENABLE");
    if (!enable || std::string_view(enable) != "strict-v5-oute2")
        GTEST_SKIP() << "Run only with the owner-only disposable v4 source";
    const auto profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto sourceV4 = slotBytes(profile / "saves/candidate-v4.ec6");
    const auto seedBefore = slotBytes(profile / "saves/autosave.ec6");
    ASSERT_FALSE(sourceV4.empty());
    ASSERT_FALSE(seedBefore.empty());
    const MapId targetId = pMapTable->GetMapInfo(TARGET_MAP);
    ASSERT_NE(targetId, MAP_INVALID);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    bool completed = false;
    std::string refusal;
    game.runGameRoutine([&] {
        try {
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto firstDecorations = sourceDecorations(maps, "oute3.odm");
            auto prior = std::unique_ptr<mm6::V4FullSessionPreflight>(
                new mm6::V4FullSessionPreflight(mm6::stageV4FullSessionPreflight(
                    sourceV4, freshNpcs, maps.expectedNames(),
                    [&maps](std::string_view name) { return maps.readBase(name); },
                    verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                    pTileTable->tileCount(), engine->config->gameplay.MaxActors.value(),
                    application->component<EngineRandomComponent>()->type(),
                    firstDecorations, GameTitle::MM6)));
            *pParty = prior->v3.represented.party.party;
            *pNPCStats = prior->v3.represented.world.world.npcs;
            *gameTimer = prior->v3.represented.world.visuals.timer;
            *pActiveOverlayList = prior->v3.represented.world.visuals.overlays;
            pMapDeltas = std::move(prior->v3.represented.world.world.mapDeltas);
            mm6::chestTitleState() = prior->v3.represented.world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables,
                prior->v3.represented.persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                prior->v3.represented.raw.saved,
                {std::move(prior->v3.represented.raw.cursor),
                 std::move(prior->v3.represented.raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(targetId, MAP_START_POINT_PARTY);
            {
                mm6::ScopedChestMapLoadOptIn chestOptIn;
                DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
            }
            const auto candidate = captureLaterOutdoor(sourceV4);
            const auto validate = [&](std::span<const std::uint8_t> proposed) {
                requireLaterOutdoorCandidate(proposed, sourceV4);
            };
            validate(candidate);
            mm6::writeAtomicV5SaveFileForTest(
                profile, TARGET_SLOT, candidate, validate);
            if (slotBytes(profile / "saves" / TARGET_SLOT) != candidate ||
                slotBytes(profile / "saves/candidate-v4.ec6") != sourceV4 ||
                slotBytes(profile / "saves/autosave.ec6") != seedBefore)
                throw std::runtime_error("MM6 later outdoor atomic write changed another slot");
            completed = true;
        } catch (const std::exception &error) {
            refusal = error.what();
        }
    });
    for (int tick = 0; tick < 256 && !completed && refusal.empty(); ++tick)
        game.tick();
    EXPECT_TRUE(completed) << refusal;
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
