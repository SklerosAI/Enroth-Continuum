#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/PartyPlacement.h"
#include "Engine/SpawnPoint.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Interaction.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6IndoorFaceLiveOwner.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "Engine/Graphics/Overlays.h"
#include "GUI/GUIWindow.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6V6IndoorProbeSlot.h"
#include "MM6V6IndoorLiveRawOwnersForTest.h"
#include "MM6IndoorSourceSweepForTest.h"

namespace {

std::string indoorProbeMap(const mm6::LoadedMapSource &maps) {
    const char *caseValue = std::getenv("ENROTH_MM6_V6_INDOOR_CASE");
    const std::string_view selected = caseValue ? std::string_view(caseValue) :
                                                 std::string_view("first");
    if (selected == "sweep-risk")
        return mm6::testing::highestRiskAdditionalIndoorCaseForTest(
            mm6::testing::sweepRetainedIndoorSourcesForTest(maps)).baseName;
    if (selected != "first" && selected != "second")
        throw std::runtime_error("MM6 v6 indoor probe requires first, second or sweep-risk case");
    const auto names = maps.expectedNames();
    const int ordinal = selected == "second" ? 2 : 1;
    auto entry = names.begin();
    for (int index = 0; index < ordinal; ++index) {
        entry = std::find_if(entry, names.end(), [](const std::string &name) {
            return name.ends_with(".dlv");
        });
        if (entry == names.end())
            break;
        if (index + 1 < ordinal)
            ++entry;
    }
    if (entry == names.end())
        throw std::runtime_error("MM6 v6 indoor probe lacks the selected source map pair");
    return entry->substr(0, entry->size() - 4) + ".blv";
}

// Select only a point derived from the loaded BLV and verified by the same
// sector/floor queries used by real indoor movement. This fallback is a
// disposable fixture when a map has no named arrival decoration.
std::optional<PartyPlacement> nativeIndoorFixturePlacement() {
    constexpr std::array<std::pair<int, Vec3f>, 4> directions = {{
        {0, Vec3f(128, 0, 0)}, {512, Vec3f(0, 128, 0)},
        {1024, Vec3f(-128, 0, 0)}, {1536, Vec3f(0, -128, 0)},
    }};
    const auto tryPoint = [&directions](Vec3f point) -> std::optional<PartyPlacement> {
        const int sector = pIndoor->GetSector(point);
        if (!sector)
            return std::nullopt;
        int face = -1;
        const float floor = BLV_GetFloorLevel(point, sector, &face);
        if (face < 0 || !std::isfinite(floor) || floor <= -30000)
            return std::nullopt;
        point.z = floor;
        if (!pIndoor->GetSector(point) ||
            !pIndoor->GetSector(point + Vec3f(0, 0, pParty->eyeLevel)))
            return std::nullopt;
        for (const auto &[yaw, step] : directions) {
            const Vec3f ahead = point + step;
            const int nextSector = pIndoor->GetSector(ahead);
            if (!nextSector ||
                !pIndoor->GetSector(ahead + Vec3f(0, 0, pParty->eyeLevel)))
                continue;
            const float nextFloor = BLV_GetFloorLevel(ahead, nextSector);
            if (std::isfinite(nextFloor) && nextFloor > -30000 &&
                std::abs(nextFloor - floor) <= 128)
                return PartyPlacement(point, yaw, 0, 0);
        }
        return std::nullopt;
    };
    for (const SpawnPoint &spawn : pIndoor->pSpawnPoints)
        if (auto placement = tryPoint(spawn.position))
            return placement;
    for (const BLVSector &sector : pIndoor->sectors) {
        for (std::uint16_t id : sector.floorIds) {
            if (id >= pIndoor->faces.size())
                continue;
            const BLVFace &face = pIndoor->faces[id];
            if (face.Ethereal() || face.vertexIds.size() < 3)
                continue;
            for (std::size_t index = 2; index < face.vertexIds.size(); ++index) {
                const int a = face.vertexIds[0];
                const int b = face.vertexIds[index - 1];
                const int c = face.vertexIds[index];
                if (a < 0 || b < 0 || c < 0 ||
                    static_cast<std::size_t>(a) >= pIndoor->vertices.size() ||
                    static_cast<std::size_t>(b) >= pIndoor->vertices.size() ||
                    static_cast<std::size_t>(c) >= pIndoor->vertices.size())
                    continue;
                const Vec3f center = (pIndoor->vertices[a] +
                    pIndoor->vertices[b] + pIndoor->vertices[c]) / 3.0f;
                if (auto placement = tryPoint(center))
                    return placement;
            }
        }
    }
    return std::nullopt;
}

} // namespace

// Parse every retained MM6 indoor base/delta pair and classify only generic
// engine ownership features. Do not print source names, dimensions or counts.
GAME_TEST(MM6IndoorSourceSweep, DISABLED_AllRetainedPairsOffline) {
    const char *enable = std::getenv("ENROTH_MM6_INDOOR_SWEEP_ENABLE");
    if (!enable || std::string_view(enable) != "strict-offline-indoor-sweep")
        GTEST_SKIP() << "Opt in to the private local MM6 indoor source sweep";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const auto sweep = mm6::testing::sweepRetainedIndoorSourcesForTest(maps);
    ASSERT_GT(sweep.variants.size(), 1u);
    const auto &candidate =
        mm6::testing::highestRiskAdditionalIndoorCaseForTest(sweep);
    ASSERT_GE(candidate.ordinal, 2u);
    ASSERT_TRUE(candidate.exactDefaultDeltaRoundTrip);
    // A native eligible source marker would need a separate generated-row and
    // positive dialogue fixture before this corpus could support admission.
    EXPECT_FALSE(sweep.hasEligibleNativeStreetNpcMarker);
    std::cout << "MM6 indoor source sweep: all retained pairs parsed; "
              << (sweep.hasCodecRefusal ? "changed-state codec has refusals; " :
                                          "changed-state codec exact; ")
              << (sweep.hasInitiallyMovingDoor ? "initial door motion present; " :
                                                 "initial door motion absent; ")
              << (sweep.hasPrevisibleOutlines ? "saved visibility present; " :
                                                "saved visibility absent; ")
              << (sweep.hasNegativeActorNpcSource ?
                  "signed actor NPC source present; " :
                  "signed actor NPC source absent; ")
              << (sweep.hasEligibleNativeStreetNpcMarker ?
                  "eligible StreetNPC source marker present; " :
                  "eligible StreetNPC source marker absent; ")
              << (sweep.hasMultiActiveDoor ? "multiple door anchors present; " :
                                             "multiple door anchors absent; ")
              << (sweep.hasActorRecords ? "actor records present; " :
                                          "actor records absent; ")
              << (sweep.hasGroundRecords ? "ground records present; " :
                                           "ground records absent; ")
              << (sweep.hasPersistentBytes ? "persistent bytes set; " :
                                            "persistent bytes clear; ")
              << "distinct risk fixture selected\n";
}

// Reconstruct the risk-ranked source through the real indoor loader, then
// stop before map events. This checkpoint retains the native signed source
// marker; the MM6 map-load policy clears it before runtime interaction.
GAME_TEST(MM6IndoorSourceSweep, DISABLED_RiskCasePreEventActorIdentity) {
    const char *enable = std::getenv("ENROTH_MM6_INDOOR_SWEEP_ENABLE");
    if (!enable || std::string_view(enable) != "strict-risk-pre-event")
        GTEST_SKIP() << "Opt in to the source-ranked MM6 indoor actor gate";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const auto sweep = mm6::testing::sweepRetainedIndoorSourcesForTest(maps);
    const std::string mapName =
        mm6::testing::highestRiskAdditionalIndoorCaseForTest(sweep).baseName;
    const MapId map = pMapTable->GetMapInfo(mapName);
    ASSERT_NE(map, MAP_INVALID);
    const auto parsedBase = deserializeMM6IndoorBase(maps.readBase(mapName));

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls())
        if (std::string_view(name).ends_with(".dlv") ||
            std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));

    bool completed = false;
    bool reached = false;
    bool aborted = false;
    bool exactNativeSignedActor = false;
    bool generatedOwnerAccepted = false;
    bool signedActorInteractable = false;
    bool signedActorUnresolved = false;
    bool interactableSignedActorUnresolved = false;
    bool signedActorResolvedAsFollower = false;
    bool interactableSignedActorDialogueLookupRefused = false;
    std::exception_ptr probeError;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
            GameTitle::MM6, [&] {
                try {
                    if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                        engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                        throw std::runtime_error(
                            "MM6 indoor risk fixture preceded world reconstruction");
                    const auto native = decodeMM6IndoorRuntimeDelta(
                        Blob::view(pIndoor->mm6DeltaBytes.data(),
                                   pIndoor->mm6DeltaBytes.size()), parsedBase);
                    if (native.actors.size() > pActors.size())
                        throw std::runtime_error(
                            "MM6 indoor risk fixture lost native actor prefix");
                    FlatHirelings roster;
                    roster.Prepare();
                    for (std::size_t slot = 0; slot < native.actors.size(); ++slot)
                        if (native.actors[slot].npcId < 0) {
                            if (pActors[slot].id != slot ||
                                pActors[slot].npcId != native.actors[slot].npcId)
                                throw std::runtime_error(
                                    "MM6 indoor risk fixture changed signed actor identity");
                            exactNativeSignedActor = true;
                            const bool interactable = CanInteractWithActor(slot);
                            signedActorInteractable |= interactable;
                            try {
                                const auto address = mm6::resolveNpcAddress(
                                    pActors[slot].npcId, *pNPCStats, roster.Size());
                                if (address.kind != mm6::NpcAddressKind::Follower ||
                                    getNPCData(pActors[slot].npcId) != roster.Get(address.index) ||
                                    getNPCType(pActors[slot].npcId) !=
                                        (roster.IsFollower(address.index) ? NPC_TYPE_QUEST :
                                                                            NPC_TYPE_HIREABLE))
                                    throw std::runtime_error(
                                        "MM6 signed actor follower dialogue lookup diverged");
                                signedActorResolvedAsFollower = true;
                            } catch (const std::out_of_range &) {
                                signedActorUnresolved = true;
                                interactableSignedActorUnresolved |= interactable;
                                try {
                                    (void) getNPCData(pActors[slot].npcId);
                                } catch (const std::out_of_range &) {
                                    interactableSignedActorDialogueLookupRefused |= interactable;
                                }
                            }
                        }
                    const auto generated = mm6::captureGeneratedNpcFields(
                        pActors, *pNPCStats, mapName, GameTitle::MM6);
                    mm6::requireGeneratedNpcActorAgreement(
                        generated, pActors, mapName, GameTitle::MM6);
                    generatedOwnerAccepted = true;
                } catch (...) {
                    probeError = std::current_exception();
                }
                reached = true;
            },
            mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
        try {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(true, 1);
        } catch (const mm6::testing::PreEventCheckpointAbortForTest &) {
            aborted = true;
        }
        completed = true;
    });
    for (int tick = 0; tick < 128 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    ASSERT_TRUE(reached);
    ASSERT_TRUE(aborted);
    ASSERT_FALSE(checkpoint->reachedMapLoad());
    ASSERT_FALSE(checkpoint->reachedInput());
    if (probeError)
        std::rethrow_exception(probeError);
    EXPECT_TRUE(exactNativeSignedActor);
    EXPECT_TRUE(generatedOwnerAccepted);
    std::cout << "MM6 signed actor pre-map diagnostic: "
              << (signedActorInteractable ? "interactable; " : "not interactable; ")
              << (signedActorUnresolved ? "transient roster lookup refused; " : "transient roster lookup in range; ")
              << (signedActorResolvedAsFollower ? "transient roster lookup agrees" : "no transient roster lookup")
              << '\n';
    // This is a pre-map marker check. The MM6 map-load loop must clear this
    // ineligible signed marker before any real interaction can occur.
    EXPECT_TRUE(interactableSignedActorUnresolved);
    EXPECT_TRUE(interactableSignedActorDialogueLookupRefused);
}

// Run alone after the disposable v2 autosave seed. This constructs a v6
// indoor candidate in memory using one real BLV/DLV and the actual pre-event
// face checkpoint. A separate opt-in path captures genuine post-world owners.
// Neither production SaveLoad nor gameplay admission is opened.
GAME_TEST(MM6V6IndoorStage, DISABLED_PrivateSourceAndPreEventOwnersStageDetached) {
    const char *enable = std::getenv("ENROTH_MM6_V6_INDOOR_STAGE_ENABLE");
    const bool liveSessionWriter = enable &&
        std::string_view(enable) == "strict-indoor-v6-live-session-write";
    const bool postWorldWriter = enable &&
        (std::string_view(enable) == "strict-indoor-v6-postworld-write" ||
         liveSessionWriter);
    const bool writeCandidate = enable &&
        (std::string_view(enable) == "strict-indoor-v6-write" || postWorldWriter);
    const std::string_view candidateSlot = liveSessionWriter ?
        mm6::testing::V6_INDOOR_LIVE_SESSION_PROBE_SLOT :
        postWorldWriter ? mm6::testing::V6_INDOOR_POSTWORLD_PROBE_SLOT :
                          mm6::testing::V6_INDOOR_PROBE_SLOT;
    if (!enable || (!writeCandidate &&
                    std::string_view(enable) != "strict-indoor-v6-stage"))
        GTEST_SKIP() << "Opt in after creating a disposable MM6 v2 seed";
    const auto profile = mm6::testing::indoorProbeProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set an owner-only system-temp MM6 restart profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto originalSeed = mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6");
    auto raw = std::unique_ptr<mm6::RawSaveFields>(
        new mm6::RawSaveFields(mm6::decodeRawSave(originalSeed, GameTitle::MM6)));
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const std::string mapName = indoorProbeMap(maps);
    const MapId map = pMapTable->GetMapInfo(mapName);
    ASSERT_NE(map, MAP_INVALID);

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls())
        if (std::string_view(name).ends_with(".dlv") ||
            std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));

    bool completed = false;
    bool reached = false;
    bool aborted = false;
    std::vector<mm6::IndoorFaceState> preEventFaces;
    std::vector<std::int16_t> preEventDoorsData;
    std::vector<std::uint8_t> activeDelta;
    mm6::GeneratedNpcSaveFields generated;
    GameplayRandomState rng;
    mm6::EventRuntimeSaveFields eventRuntime;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    const auto parsedBase = deserializeMM6IndoorBase(maps.readBase(mapName));
    const auto projectedBase = projectMM6IndoorBase(parsedBase);
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
            GameTitle::MM6,
            [&] {
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 v6 indoor checkpoint preceded map reconstruction");
                preEventFaces = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, projectedBase, GameTitle::MM6);
                preEventDoorsData = pIndoor->doorsData;
                activeDelta = pIndoor->mm6DeltaBytes;
                generated = mm6::captureGeneratedNpcFields(
                    pActors, *pNPCStats, mapName, GameTitle::MM6);
                rng = mm6::captureGameplayRngFields(
                    *application->component<EngineRandomComponent>(), GameTitle::MM6);
                mm6::EventRuntimeMapEntry active;
                active.mapName = mapName;
                active.respawnCount = pIndoor->dlv.respawnCount;
                active.lastRespawnDay = pIndoor->dlv.lastRespawnDay;
                active.mapVars = engine->_persistentVariables.mapVars;
                active.decorVars = engine->_persistentVariables.decorVars;
                active.bindings = mm6::captureEventRuntimeDecorationBindings(
                    pLevelDecorations, GameTitle::MM6);
                eventRuntime.maps.push_back(std::move(active));
                reached = true;
            },
            mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
        try {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(true, 1);
        } catch (const mm6::testing::PreEventCheckpointAbortForTest &) {
            aborted = true;
        }
        completed = true;
    });
    for (int tick = 0; tick < 128 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    ASSERT_TRUE(aborted);
    ASSERT_TRUE(reached);
    ASSERT_FALSE(checkpoint->reachedMapLoad());
    ASSERT_FALSE(checkpoint->reachedInput());
    ASSERT_FALSE(preEventFaces.empty());
    ASSERT_FALSE(activeDelta.empty());

    const std::string deltaName = mapName.substr(0, mapName.size() - 4) + ".dlv";
    raw->metadata.header.mapFileName = mapName;
    raw->chest.activeDeltaName = deltaName;
    raw->chest.rawActiveDelta = activeDelta;
    const auto it = std::find_if(raw->maps.deltas.begin(), raw->maps.deltas.end(),
        [&deltaName](const mm6::SavedMapDelta &entry) { return entry.name == deltaName; });
    ASSERT_NE(it, raw->maps.deltas.end());
    it->bytes = activeDelta;
    auto rawBytes = mm6::encodeRawSave(*raw, GameTitle::MM6);

    std::unique_ptr<mm6::RawSaveFields> postWorldRaw;
    std::vector<Actor> postWorldActors;
    std::vector<SpriteObject> postWorldSprites;
    std::vector<LevelDecoration> postWorldDecorations;
    std::vector<mm6::IndoorFaceState> postWorldFaces;
    bool clearedSignedNativePostWorld = false;

    // A separate opt-in candidate observes the real NPC generation and map
    // reload boundary. The original fixture above remains a pre-event probe.
    if (postWorldWriter) {
        bool worldFinished = false;
        std::exception_ptr worldError;
        game.runGameRoutine([&] {
            try {
                checkpoint.reset();
                const auto freshNpcs = mm6::loadNpcStaticBaseline(
                    *engine->resources(), GameTitle::MM6);
                const auto verifyItem = [](std::int32_t id) {
                    return id >= 1 && id <= 580 &&
                        !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
                };
                auto represented = std::unique_ptr<mm6::StagedRawSessionFields>(
                    new mm6::StagedRawSessionFields(mm6::stageRawSessionFields(
                        rawBytes, freshNpcs, maps.expectedNames(),
                        [&maps](std::string_view name) { return maps.readBase(name); },
                        verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                        pTileTable->tileCount(), GameTitle::MM6)));
                *pParty = represented->party.party;
                mm6::chestTitleState() = represented->world.chestTitle;
                if (liveSessionWriter) {
                    *pNPCStats = represented->world.world.npcs;
                    *gameTimer = represented->world.visuals.timer;
                    *pActiveOverlayList = represented->world.visuals.overlays;
                    pMapDeltas.clear();
                    for (const auto &[name, blob] : represented->world.world.mapDeltas)
                        pMapDeltas.emplace(name, Blob::copy(blob));
                    mm6::publishFreshChildPersistentEventZeros(
                        &engine->_persistentVariables,
                        represented->persistentEvents, GameTitle::MM6);
                    mm6::publishRawCursorShopOwners(
                        &mm6::rawHeldCursorState(GameTitle::MM6),
                        &mm6::shopTitleState(), represented->raw.saved,
                        {represented->raw.cursor, represented->raw.shops},
                        GameTitle::MM6);
                }
                engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
                engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
                mm6::ScopedChestMapLoadOptIn chestOptIn(true);
                std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> placementCheckpoint;
                if (liveSessionWriter) {
                    placementCheckpoint = std::make_unique<
                        mm6::testing::ScopedPreEventCheckpointForTest>(
                            GameTitle::MM6, [&] {
                                // A loaded save preserves its Party position. The v2
                                // seed belongs to an outdoor map, so this disposable
                                // indoor fixture must take the map's actual start
                                // marker before OnMapReload or first input.
                                std::optional<PartyPlacement> placement;
                                for (MapStartPoint start : {
                                        MAP_START_POINT_PARTY, MAP_START_POINT_NORTH,
                                        MAP_START_POINT_SOUTH, MAP_START_POINT_EAST,
                                        MAP_START_POINT_WEST}) {
                                    auto candidate = MapDestination(map, start).resolvePlacement();
                                    if (candidate && pIndoor->GetSector(candidate->pos) &&
                                        pIndoor->GetSector(candidate->pos +
                                            Vec3f(0, 0, pParty->eyeLevel))) {
                                        placement = std::move(candidate);
                                        break;
                                    }
                                }
                                if (!placement)
                                    placement = nativeIndoorFixturePlacement();
                                if (!placement)
                                    throw std::runtime_error(
                                        "MM6 v6 indoor fixture lacks a verified native floor point");
                                placeParty(*placement);
                                if (!pIndoor->GetSector(pParty->pos) ||
                                    !pIndoor->GetSector(
                                        pParty->pos + Vec3f(0, 0, pParty->eyeLevel)))
                                    throw std::runtime_error(
                                        "MM6 v6 indoor fixture Party Start has no eye sector");
                            });
                }
                DoPrepareWorld(true, 1);
                if (liveSessionWriter && !placementCheckpoint->reachedMapLoad())
                    throw std::runtime_error(
                        "MM6 v6 indoor fixture missed Party Start checkpoint");
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 v6 post-world writer lacked indoor world");
                const auto nativeAtEntry = decodeMM6IndoorRuntimeDelta(
                    Blob::view(activeDelta.data(), activeDelta.size()), parsedBase);
                for (std::size_t slot = 0; slot < nativeAtEntry.actors.size(); ++slot) {
                    const auto &nativeActor = nativeAtEntry.actors[slot];
                    if (!mm6::clearsNativeActorNpc(
                            static_cast<MonsterId>(nativeActor.monsterInfo.id),
                            nativeActor.npcId))
                        continue;
                    if (slot >= pActors.size() || pActors[slot].npcId != 0)
                        throw std::runtime_error(
                            "MM6 post-world actor retained an ineligible native NPC marker");
                    clearedSignedNativePostWorld = true;
                }
                generated = mm6::captureGeneratedNpcFields(
                    pActors, *pNPCStats, mapName, GameTitle::MM6);
                rng = mm6::captureGameplayRngFields(
                    *application->component<EngineRandomComponent>(), GameTitle::MM6);
                auto &active = eventRuntime.maps.front();
                active.mapVars = engine->_persistentVariables.mapVars;
                active.decorVars = engine->_persistentVariables.decorVars;
                active.bindings = mm6::captureEventRuntimeDecorationBindings(
                    pLevelDecorations, GameTitle::MM6);
                if (liveSessionWriter) {
                    mm6::testing::requireV6IndoorNativeResidualsForTest(
                        parsedBase, preEventDoorsData);
                    postWorldRaw = mm6::testing::captureV6IndoorLiveRawOwnersForTest(
                        *raw, mapName);
                }
                postWorldActors.assign(pActors.begin(), pActors.end());
                postWorldSprites = pSpriteObjects;
                postWorldDecorations = pLevelDecorations;
                postWorldFaces = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, projectedBase, GameTitle::MM6);
            } catch (...) {
                worldError = std::current_exception();
            }
            worldFinished = true;
        });
        for (int tick = 0; tick < 128 && !worldFinished; ++tick)
            game.tick();
        ASSERT_TRUE(worldFinished);
        if (worldError)
            std::rethrow_exception(worldError);
        if (liveSessionWriter) {
            if (!postWorldRaw)
                throw std::runtime_error("MM6 v6 post-world raw owners were not captured");
            *raw = std::move(*postWorldRaw);
            rawBytes = mm6::encodeRawSave(*raw, GameTitle::MM6);
        }
    }

    if (postWorldWriter && std::getenv("ENROTH_MM6_V6_INDOOR_CASE") &&
        std::string_view(std::getenv("ENROTH_MM6_V6_INDOOR_CASE")) == "sweep-risk")
        EXPECT_TRUE(clearedSignedNativePostWorld);

    bool stageFinished = false;
    std::exception_ptr stageError;
    std::unique_ptr<mm6::StagedV6IndoorSaveFields> staged;
    std::vector<std::uint8_t> candidateBytes;
    game.runGameRoutine([&] {
        try {
            const auto freshNpcs = mm6::loadNpcStaticBaseline(
                *engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto represented = std::unique_ptr<mm6::StagedRawSessionFields>(
                new mm6::StagedRawSessionFields(mm6::stageRawSessionFields(
                rawBytes, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), GameTitle::MM6)));
            if (!represented->indoorPlan || represented->outdoorPlan)
                throw std::runtime_error("MM6 v6 probe did not stage an indoor raw world");
            if (liveSessionWriter)
                mm6::testing::requireV6IndoorDetachedRawRoundTripForTest(
                    *raw, *represented, *pItemTable,
                    pOverlayTable->overlays.size());
            auto saved = std::make_unique<mm6::V6IndoorSaveFields>();
            saved->raw = std::move(*raw);
            saved->generatedNpcs = std::move(generated);
            saved->gameplayRng = std::move(rng);
            saved->eventRuntime = std::move(eventRuntime);
            mm6::captureV6IndoorNativeAnchors(
                *represented, &saved->actors, &saved->sprites, &saved->decorations);
            const std::vector<Actor> liveActors = postWorldWriter ?
                postWorldActors : std::vector<Actor>(pActors.begin(), pActors.end());
            saved->runtimeActors = mm6::captureIndoorActorRuntime(
                mapName, saved->actors, liveActors, GameTitle::MM6);
            saved->runtimeSprites = mm6::captureIndoorSpriteRuntime(
                mapName, saved->sprites,
                postWorldWriter ? postWorldSprites : pSpriteObjects, GameTitle::MM6);
            auto changedDecorations = postWorldWriter ?
                postWorldDecorations : pLevelDecorations;
            if (changedDecorations.empty())
                throw std::runtime_error("MM6 v6 IDRT probe has no decoration slot");
            if (!postWorldWriter)
                changedDecorations.front().uFlags = LevelDecorationFlags(
                    std::to_underlying(changedDecorations.front().uFlags) ^ 0x20u);
            const auto event = std::find_if(saved->eventRuntime.maps.begin(),
                                            saved->eventRuntime.maps.end(),
                [&mapName](const mm6::EventRuntimeMapEntry &entry) {
                    return entry.mapName == mapName;
                });
            if (event == saved->eventRuntime.maps.end())
                throw std::runtime_error("MM6 v6 IDRT probe lacks active EVRT map");
            saved->runtimeDecorations = mm6::captureIndoorDecorationRuntime(
                mapName, represented->indoorPlan->base,
                *event, *pDecorationTable,
                changedDecorations, GameTitle::MM6);
            auto changed = postWorldWriter ? postWorldFaces : preEventFaces;
            if (!postWorldWriter)
                changed.front().attributes ^= 1u;
            if (auto overrides = mm6::captureIndoorFaceOverrides(
                    mapName, preEventFaces, changed, GameTitle::MM6))
                saved->faceOverrides.maps.push_back(std::move(*overrides));
            candidateBytes = mm6::encodeV6IndoorSave(*saved, GameTitle::MM6);
            const auto readFaces = [&mapName, &preEventFaces](std::string_view requested) {
                if (requested != mapName)
                    throw std::runtime_error("MM6 v6 probe requested another face source");
                return preEventFaces;
            };
            staged.reset(new mm6::StagedV6IndoorSaveFields(mm6::stageV6IndoorSave(
                candidateBytes, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, *pDecorationTable,
                pOverlayTable->overlays.size(),
                pTileTable->tileCount(), readFaces, GameTitle::MM6)));
            if (liveSessionWriter)
                mm6::testing::requireV6IndoorDetachedRawRoundTripForTest(
                    staged->saved.raw, staged->represented, *pItemTable,
                    pOverlayTable->overlays.size());
        } catch (...) {
            stageError = std::current_exception();
        }
        stageFinished = true;
    });
    for (int tick = 0; tick < 128 && !stageFinished; ++tick)
        game.tick();
    ASSERT_TRUE(stageFinished);
    if (stageError)
        std::rethrow_exception(stageError);
    ASSERT_TRUE(staged);
    ASSERT_TRUE(staged->represented.indoorPlan);
    if (!postWorldWriter)
        EXPECT_EQ(staged->saved.faceOverrides.maps.size(), 1u);
    EXPECT_FALSE(staged->canEnterGameLoop());
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), originalSeed);
    if (writeCandidate) {
        mm6::testing::writeIndoorProbeCandidate(profile, candidateBytes, candidateSlot);
        EXPECT_EQ(mm6::testing::readIndoorProbeSlot(
            profile, candidateSlot), candidateBytes);
        EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), originalSeed);
    }
}
