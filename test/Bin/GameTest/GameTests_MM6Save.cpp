#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6KnownSaveFields.h"
#include "Engine/MM6/MM6HeldItemSaveFields.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NewGameSaveRuntime.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PrivateMapSource.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/MM6/MM6WorldSaveRuntime.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIGame.h"
#include "Io/Mouse.h"

namespace {

class TempProfile {
 public:
    TempProfile() {
        std::string pattern = (std::filesystem::temp_directory_path() / "mm6-headless-save-XXXXXX").string();
        char *created = ::mkdtemp(pattern.data());
        if (!created)
            throw std::runtime_error("cannot create temporary MM6 save profile");
        root = created;
    }

    ~TempProfile() {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }

    std::filesystem::path primary() const { return root / "saves" / "autosave.ec6"; }
    std::filesystem::path backup() const { return root / "saves" / "autosave.ec6.bak"; }

    std::filesystem::path root;
};

std::vector<std::uint8_t> readFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::unordered_map<std::string, std::string> snapshotCache() {
    std::unordered_map<std::string, std::string> snapshot;
    for (const auto &[name, blob] : pMapDeltas)
        snapshot.emplace(name, blob.str());
    return snapshot;
}

std::optional<std::filesystem::path> restartProfileRoot() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return std::nullopt;
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 restart profile must be an existing absolute directory");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-"))
        throw std::runtime_error("MM6 restart profile must be a disposable system-temp directory");
    constexpr auto otherPermissions = std::filesystem::perms::group_all |
                                      std::filesystem::perms::others_all;
    if ((std::filesystem::status(root).permissions() & otherPermissions) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 restart profile must be owner-only");
    return root;
}

} // namespace

GAME_TEST(MM6ZSaveHeadless, FreshNpcBaselineMatchesLoadedMm6Tables) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(engine->resources(), nullptr);
    ASSERT_NE(pNPCStats, nullptr);
    const std::string priorTopic = pNPCTopics[0].pText;
    const auto baseline = mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
    EXPECT_GT(baseline.stats().uNumNewNPCs, 0);
    EXPECT_LE(baseline.stats().uNumNewNPCs, pNPCStats->pOriginalNPCData.size());
    for (std::size_t i = 1; i < baseline.stats().pOriginalNPCData.size(); ++i) {
        const NPCData &fresh = baseline.stats().pOriginalNPCData[i];
        const NPCData &loaded = pNPCStats->pOriginalNPCData[i];
        EXPECT_TRUE(fresh.name == loaded.name) << i;
        EXPECT_EQ(fresh.portraitId, loaded.portraitId) << i;
        EXPECT_EQ(fresh.profession, loaded.profession) << i;
    }
    EXPECT_EQ(baseline.stats().pProfessionChance[MAP_EMERALD_ISLAND].total,
              pNPCStats->pProfessionChance[MAP_EMERALD_ISLAND].total);
    EXPECT_TRUE(pNPCTopics[0].pText == priorTopic);
}

GAME_TEST(MM6ZSaveHeadless, DetachedPartyConstructionPreservesLiveCursor) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_TRUE(mouse);
    const std::string previousName = mouse->cursor_name;
    const int previousInitialized = mouse->bInitialized;
    mouse->cursor_name = "detached-party-sentinel";
    mouse->bInitialized = 1;
    Party detached;
    EXPECT_EQ(mouse->cursor_name, "detached-party-sentinel");
    EXPECT_EQ(detached.pPickedItem.itemId, ITEM_NULL);
    mouse->cursor_name = previousName;
    mouse->bInitialized = previousInitialized;
}

GAME_TEST(MM6ZSaveHeadless, LoadedMapSourceMatchesPrivateResearch) {
    const char *research = std::getenv("ENROTH_MM6_MAP_RESEARCH_ROOT");
    if (!research || !std::filesystem::is_directory(research))
        GTEST_SKIP() << "Set ENROTH_MM6_MAP_RESEARCH_ROOT to the ignored private F3 map source";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    const mm6::PrivateMapSource privateMaps(research);
    const mm6::LoadedMapSource loaded(*pGames_LOD, *pMapTable, GameTitle::MM6);
    ASSERT_EQ(loaded.expectedNames().size(), privateMaps.expectedNames().size());
    EXPECT_TRUE(std::equal(loaded.expectedNames().begin(), loaded.expectedNames().end(),
                           privateMaps.expectedNames().begin()));
    for (std::size_t index = 0; index < loaded.expectedNames().size(); ++index) {
        const std::string &delta = loaded.expectedNames()[index];
        const std::string base = delta.substr(0, delta.size() - 4) +
            (delta.ends_with(".dlv") ? ".blv" : ".odm");
        try {
            const Blob original = loaded.readBase(base);
            const Blob retained = privateMaps.readBase(base);
            EXPECT_EQ(original.size(), retained.size());
            EXPECT_TRUE(original.str() == retained.str());
            EXPECT_FALSE(loaded.readDefaultDelta(delta).empty());
        } catch (const std::exception &error) {
            FAIL() << "MM6 loaded map source failed at pair " << index << ": " << error.what();
            return;
        }
    }
}

GAME_TEST(MM6ZSaveHeadless, NewGameCallbackFailureRestoresPlacement) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId priorMap = engine->_currentLoadedMapId;
    const auto priorPosition = pParty->pos;
    const auto priorFallStartZ = pParty->uFallStartZ;
    const auto priorPitch = pParty->_viewPitch;
    const auto priorYaw = pParty->_viewYaw;
    const std::string originalSaveName = engine->_lastLoadedSaveFileName;
    engine->_lastLoadedSaveFileName = "prior-test-slot";

    mm6::testing::setNewGameSaveOverride([] { throw std::runtime_error("test callback failure"); });
    EXPECT_THROW(saveNewGame(), std::runtime_error);
    mm6::testing::setNewGameSaveOverride({});

    EXPECT_EQ(engine->_currentLoadedMapId, priorMap);
    EXPECT_EQ(pParty->pos.x, priorPosition.x);
    EXPECT_EQ(pParty->pos.y, priorPosition.y);
    EXPECT_EQ(pParty->pos.z, priorPosition.z);
    EXPECT_EQ(pParty->uFallStartZ, priorFallStartZ);
    EXPECT_EQ(pParty->_viewPitch, priorPitch);
    EXPECT_EQ(pParty->_viewYaw, priorYaw);
    EXPECT_EQ(engine->_lastLoadedSaveFileName, "prior-test-slot");
    engine->_lastLoadedSaveFileName = originalSaveName;
}

GAME_TEST(MM6ZSaveHeadless, ScopedRealFirstMapSaveAndRollback) {
    const char *research = std::getenv("ENROTH_MM6_MAP_RESEARCH_ROOT");
    if (!research || !std::filesystem::is_directory(research))
        GTEST_SKIP() << "Set ENROTH_MM6_MAP_RESEARCH_ROOT to the ignored private F3 map source";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    TempProfile profile;

    struct Observation {
        bool firstCallback = false;
        bool failureCallback = false;
        bool success = false;
        bool rollbackExact = false;
        bool queuedDuringPrecommitRejected = false;
        bool freshDiskStage = false;
        bool detachedOutdoorPlan = false;
        bool idleHeldCursorPlan = false;
        bool wrongTitleRejected = false;
        bool corruptPrimaryRecoveredReadOnly = false;
        std::string error;
        std::uint32_t firstSeed = 0;
        std::uint32_t secondSeed = 0;
        std::vector<std::uint8_t> primaryBeforeFailure;
        std::vector<std::uint8_t> backupBeforeFailure;
        std::vector<std::uint8_t> primaryAfterFailure;
        std::vector<std::uint8_t> backupAfterFailure;
    } result;

    game.runGameRoutine([&] {
        const Party oldParty = *pParty;
        const ActiveOverlayList oldOverlays = *pActiveOverlayList;
        const auto oldChest = mm6::chestTitleState();
        std::unordered_map<std::string, Blob> oldCache;
        for (const auto &[name, blob] : pMapDeltas)
            oldCache.emplace(name, Blob::share(blob));
        const auto restore = [&] {
            *pParty = oldParty;
            *pActiveOverlayList = oldOverlays;
            pMapDeltas = std::move(oldCache);
            mm6::chestTitleState() = oldChest;
        };
        try {
            pParty->Reset();
            for (Character &character : pParty->pCharacters)
                character.name = "Hero";
            pActiveOverlayList->Reset();
            pMapDeltas.clear();
            mm6::resetChestTitleState(&mm6::chestTitleState());
            const auto actions = mm6::capturePendingActionStateForSave(GameTitle::MM6);
            const auto cursorPlan = mm6::stageHeldCursorRestorePlan(
                {}, *pItemTable, actions, GameTitle::MM6);
            result.idleHeldCursorPlan = cursorPlan.iconName == "MICON1" &&
                !cursorPlan.itemCursor && cursorPlan.resetClickOffset;

            (void) mm6::saveNewGameMM6ScopedForTest(profile.root, research,
                [&](const Party &staged, const std::unordered_map<std::string, Blob> &cache,
                    const mm6::ChestTitleState &chest) {
                    result.firstCallback = staged.pos.z == 340 && cache.size() == 67 &&
                        cache.contains("oute3.ddm") && chest.ready;
                });
            result.firstSeed = mm6::chestTitleState().lcgState;
            // A second successful write creates both a primary and a backup.
            (void) mm6::saveNewGameMM6ScopedForTest(profile.root, research,
                [&](const Party &staged, const std::unordered_map<std::string, Blob> &cache,
                    const mm6::ChestTitleState &chest) {
                    result.success = staged.pos.z == 340 && cache.size() == 67 && chest.ready;
                });
            result.secondSeed = mm6::chestTitleState().lcgState;
            result.primaryBeforeFailure = readFile(profile.primary());
            result.backupBeforeFailure = readFile(profile.backup());
            const mm6::PrivateMapSource maps(research);
            Party freshParty;
            freshParty.uNumGold = 27;
            NPCStats freshNpcs{};
            Timer freshTimer;
            ActiveOverlayList freshOverlays;
            const auto readBase = [&maps](std::string_view name) { return maps.readBase(name); };
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto staged = mm6::stageOfflineSaveSlotForTest(
                profile.root, mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
                freshTimer, freshOverlays, maps.expectedNames(), readBase,
                verifyItem, pOverlayTable->overlays.size(), GameTitle::MM6);
            result.freshDiskStage = staged && !staged->needsRecovery &&
                staged->bytes == result.primaryBeforeFailure &&
                staged->staged.party.pos.z == 340 &&
                staged->staged.worldAndVisuals.world.mapDeltas.size() == 67 &&
                staged->staged.worldAndVisuals.rawActiveDelta ==
                    staged->staged.worldAndVisuals.known.chest.rawActiveDelta &&
                staged->staged.worldAndVisuals.chestTitle.ready &&
                staged->staged.worldAndVisuals.chestTitle.lcgState == result.secondSeed &&
                staged->staged.shopGuildClocks.ready && freshParty.uNumGold == 27 &&
                readFile(profile.primary()) == result.primaryBeforeFailure;
            if (staged) {
                const auto plan = mm6::prepareDetachedOutdoorWorldPlan(
                    staged->staged.worldAndVisuals, pTileTable->tileCount(), GameTitle::MM6);
                result.detachedOutdoorPlan = plan.mapName == "oute3.odm" &&
                    !plan.base.models.empty() && !plan.faceVertexIds.empty() &&
                    !plan.vertices.empty() &&
                    plan.spawnPoints.size() == plan.base.spawnPoints.size() &&
                    plan.activeSidecar.wire.bytes ==
                        staged->staged.worldAndVisuals.rawActiveDelta &&
                    plan.savedDelta.actors.size() == plan.activeSidecar.actors.size();
            }
            try {
                (void) mm6::stageOfflineSaveSlotForTest(
                    profile.root, mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
                    freshTimer, freshOverlays, maps.expectedNames(), readBase,
                    verifyItem, pOverlayTable->overlays.size(), GameTitle::MM7);
            } catch (const std::runtime_error &) {
                result.wrongTitleRejected = readFile(profile.primary()) == result.primaryBeforeFailure;
            }
            {
                std::ofstream corrupt(profile.primary(), std::ios::binary | std::ios::trunc);
                corrupt << "broken";
            }
            const auto corruptPrimary = readFile(profile.primary());
            auto recovered = mm6::stageOfflineSaveSlotForTest(
                profile.root, mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
                freshTimer, freshOverlays, maps.expectedNames(), readBase,
                verifyItem, pOverlayTable->overlays.size(), GameTitle::MM6);
            result.corruptPrimaryRecoveredReadOnly = recovered && recovered->needsRecovery &&
                recovered->bytes == result.backupBeforeFailure &&
                recovered->staged.worldAndVisuals.world.mapDeltas.size() == 67 &&
                recovered->staged.worldAndVisuals.chestTitle.ready &&
                readFile(profile.primary()) == corruptPrimary &&
                readFile(profile.backup()) == result.backupBeforeFailure &&
                freshParty.uNumGold == 27;
            {
                std::ofstream restorePrimary(profile.primary(), std::ios::binary | std::ios::trunc);
                restorePrimary.write(reinterpret_cast<const char *>(result.primaryBeforeFailure.data()),
                                     result.primaryBeforeFailure.size());
            }
            const auto cacheBeforeFailure = snapshotCache();
            const auto chestBeforeFailure = mm6::chestTitleState();
            try {
                (void) mm6::saveNewGameMM6ScopedForTest(profile.root, research,
                    [&](const Party &staged, const std::unordered_map<std::string, Blob> &cache,
                        const mm6::ChestTitleState &chest) {
                        result.failureCallback = staged.pos.z == 340 && cache.size() == 67 && chest.ready;
                        throw std::runtime_error("synthetic first-map entry failure");
                    });
                result.error = "scoped MM6 save did not report callback failure";
            } catch (const std::runtime_error &error) {
                if (std::string(error.what()) != "synthetic first-map entry failure")
                    result.error = error.what();
            }
            result.primaryAfterFailure = readFile(profile.primary());
            result.backupAfterFailure = readFile(profile.backup());
            result.rollbackExact = snapshotCache() == cacheBeforeFailure &&
                mm6::chestTitleState().ready == chestBeforeFailure.ready &&
                mm6::chestTitleState().lcgState == chestBeforeFailure.lcgState &&
                mm6::chestTitleState().generatedArtifactFlags == chestBeforeFailure.generatedArtifactFlags;
            try {
                (void) mm6::saveNewGameMM6ScopedForTest(profile.root, research,
                    [&](const Party &, const std::unordered_map<std::string, Blob> &,
                        const mm6::ChestTitleState &) {
                        engine->_messageQueue->addMessageNextFrame(UIMSG_OpenInventory);
                    });
                result.error = "scoped MM6 save accepted a queued precommit action";
            } catch (const std::runtime_error &) {
                result.queuedDuringPrecommitRejected =
                    engine->_messageQueue->hasPendingForSave() &&
                    readFile(profile.primary()) == result.primaryBeforeFailure &&
                    readFile(profile.backup()) == result.backupBeforeFailure &&
                    snapshotCache() == cacheBeforeFailure &&
                    mm6::chestTitleState().lcgState == chestBeforeFailure.lcgState;
            }
            engine->_messageQueue->clearAll();
        } catch (const std::exception &error) {
            result.error = error.what();
        }
        restore();
    });

    ASSERT_TRUE(result.error.empty()) << result.error;
    EXPECT_TRUE(result.firstCallback);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.failureCallback);
    EXPECT_TRUE(result.freshDiskStage);
    EXPECT_TRUE(result.detachedOutdoorPlan);
    EXPECT_TRUE(result.idleHeldCursorPlan);
    EXPECT_TRUE(result.wrongTitleRejected);
    EXPECT_TRUE(result.corruptPrimaryRecoveredReadOnly);
    EXPECT_FALSE(result.primaryBeforeFailure.empty());
    EXPECT_FALSE(result.backupBeforeFailure.empty());
    EXPECT_EQ(mm6::decodeKnownSave(result.backupBeforeFailure).chest.lcgState, result.firstSeed);
    EXPECT_EQ(mm6::decodeKnownSave(result.primaryBeforeFailure).chest.lcgState, result.secondSeed);
    EXPECT_EQ(result.primaryAfterFailure, result.primaryBeforeFailure);
    EXPECT_EQ(result.backupAfterFailure, result.backupBeforeFailure);
    EXPECT_TRUE(result.rollbackExact);
    EXPECT_TRUE(result.queuedDuringPrecommitRejected);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// Run alone in a fresh headless process. DoPrepareWorld changes renderer and
// process-global owners that the scoped .ec6 writer cannot yet roll back.
GAME_TEST(MM6ZSaveHeadless, DISABLED_ScopedSaveFeedsFirstOutdoorWorld) {
    const char *research = std::getenv("ENROTH_MM6_MAP_RESEARCH_ROOT");
    if (!research || !std::filesystem::is_directory(research))
        GTEST_SKIP() << "Set ENROTH_MM6_MAP_RESEARCH_ROOT to the ignored private F3 map source";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    TempProfile profile;

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);

    bool completed = false;
    bool hadNoHouseNpc = false;
    std::string error;
    game.runGameRoutine([&] {
        try {
            pParty->Reset();
            for (Character &character : pParty->pCharacters)
                character.name = "Hero";
            pActiveOverlayList->Reset();
            pMapDeltas.clear();
            mm6::resetChestTitleState(&mm6::chestTitleState());
            hadNoHouseNpc = std::any_of(pNPCStats->pNPCData.begin(), pNPCStats->pNPCData.end(),
                [](const NPCData &npc) { return std::to_underlying(npc.house) == -1; });
            (void) mm6::saveNewGameMM6ScopedForTest(profile.root, research,
                [](const Party &, const std::unordered_map<std::string, Blob> &,
                   const mm6::ChestTitleState &) {});
            engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
            {
                mm6::ScopedChestMapLoadOptIn chestOptIn;
                DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
            }
        } catch (const std::exception &exception) {
            error = exception.what();
        }
        completed = true;
    });
    for (int tick = 0; tick < 256 && !completed; ++tick)
        game.tick();

    ASSERT_TRUE(completed);
    ASSERT_TRUE(error.empty()) << error;
    EXPECT_TRUE(hadNoHouseNpc);
    EXPECT_TRUE(std::filesystem::exists(profile.primary()));
    EXPECT_EQ(pMapDeltas.size(), 67u);
    EXPECT_TRUE(mm6::chestTitleState().ready);
    EXPECT_EQ(engine->_currentLoadedMapId, firstMap);
    EXPECT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    EXPECT_FALSE(engine->_pendingTransition.has_value());
    EXPECT_FALSE(pOutdoor->faces.empty());
    EXPECT_FALSE(pActors.empty());
    EXPECT_EQ(vChests.size(), 20u);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// Runs the actual PartyCreation_OK -> saveNewGame -> gameLoop transition in a
// dedicated headless process. The override writes only into TempProfile;
// normal MM6 SaveLoad still rejects user sessions and reloads.
GAME_TEST(MM6ZSaveHeadless, DISABLED_NewGameControlFlowReachesFirstFrame) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    std::unique_ptr<TempProfile> temporary;
    const auto restartRoot = restartProfileRoot();
    if (!restartRoot)
        temporary = std::make_unique<TempProfile>();
    const std::filesystem::path profileRoot = restartRoot ? *restartRoot : temporary->root;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;

    mm6::testing::setNewGameSaveOverride([&] {
        (void) mm6::saveNewGameMM6FromLoadedMapsScopedForTest(profileRoot,
            [](const Party &, const std::unordered_map<std::string, Blob> &,
               const mm6::ChestTitleState &) {});
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine([] {
        for (Character &character : pParty->pCharacters) {
            for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
                const Skill skill = character.GetSkillIdxByOrder(order);
                if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                    character.pActiveSkills[skill] = CombinedSkillValue::novice();
            }
            if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
                throw std::runtime_error("MM6 creation party lacks four skills");
        }
        for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass) {
            for (Character &character : pParty->pCharacters)
                for (Attribute stat : allStatAttributes())
                    character.IncreaseAttribute(stat);
        }
        if (CharacterCreation_GetUnspentAttributePointCount() != 0)
            throw std::runtime_error("MM6 creation party could not spend attribute points");
    });
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});

    EXPECT_TRUE(std::filesystem::exists(profileRoot / "saves" / "autosave.ec6"));
    EXPECT_EQ(pMapDeltas.size(), 67u);
    EXPECT_TRUE(mm6::chestTitleState().ready);
    EXPECT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    EXPECT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(pOutdoor->faces.empty());
    EXPECT_FALSE(pActors.empty());
    EXPECT_EQ(vChests.size(), 20u);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// Run in a new process after DISABLED_NewGameControlFlowReachesFirstFrame with
// the same disposable ENROTH_MM6_RESTART_PROFILE. This stages only owned data;
// it does not assign the saved Party or enter the live world.
GAME_TEST(MM6ZSaveHeadless, DISABLED_RestartSlotStagesDetachedFirstMap) {
    const auto profileRoot = restartProfileRoot();
    if (!profileRoot)
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a disposable system-temp directory";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    const std::filesystem::path primary = *profileRoot / "saves" / "autosave.ec6";
    ASSERT_TRUE(std::filesystem::is_regular_file(primary));
    const auto before = readFile(primary);

    bool completed = false;
    bool stagedCorrectly = false;
    std::string error;
    game.runGameRoutine([&] {
        try {
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            Party freshParty;
            NPCStats freshNpcs{};
            Timer freshTimer;
            ActiveOverlayList freshOverlays;
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto staged = mm6::stageOfflineSaveSlotForTest(
                *profileRoot, mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
                freshTimer, freshOverlays, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, pOverlayTable->overlays.size(), GameTitle::MM6);
            if (!staged)
                throw std::runtime_error("MM6 restart slot was absent after first process");
            int firstCharacterItems = 0;
            for (const auto &entry : staged->staged.party.pCharacters[0].inventory.entries()) {
                (void) entry;
                ++firstCharacterItems;
            }
            const auto plan = mm6::prepareDetachedOutdoorWorldPlan(
                staged->staged.worldAndVisuals, pTileTable->tileCount(), GameTitle::MM6);
            stagedCorrectly = !staged->needsRecovery && staged->bytes == before &&
                staged->staged.party.pos.z == 340 &&
                staged->staged.worldAndVisuals.world.mapDeltas.size() == 67 &&
                staged->staged.worldAndVisuals.chestTitle.ready &&
                firstCharacterItems > 0 && plan.mapName == "oute3.odm" &&
                !plan.vertices.empty() && !plan.spawnPoints.empty();
        } catch (const std::exception &exception) {
            error = exception.what();
        }
        completed = true;
    });
    for (int tick = 0; tick < 256 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    ASSERT_TRUE(error.empty()) << error;
    EXPECT_TRUE(stagedCorrectly);
    EXPECT_EQ(readFile(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// Run in a disposable fresh process after the two tests above. This probes
// live reentry from the partial new-game slot; it must never become loadGame's
// production commit path without completing every owner and rollback gate.
GAME_TEST(MM6ZSaveHeadless, DISABLED_RestartSlotEntersFirstOutdoorWorld) {
    const auto profileRoot = restartProfileRoot();
    if (!profileRoot)
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a disposable system-temp directory";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    const std::filesystem::path primary = *profileRoot / "saves" / "autosave.ec6";
    ASSERT_TRUE(std::filesystem::is_regular_file(primary));
    const auto before = readFile(primary);

    bool completed = false;
    bool enteredWorld = false;
    std::string error;
    game.runGameRoutine([&] {
        try {
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            Party freshParty;
            // Saved NPC fields overlay the freshly loaded source tables;
            // name pools and other static resources are not EC6 fields.
            NPCStats freshNpcs = *pNPCStats;
            Timer freshTimer;
            ActiveOverlayList freshOverlays;
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto staged = mm6::stageOfflineSaveSlotForTest(
                *profileRoot, mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
                freshTimer, freshOverlays, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, pOverlayTable->overlays.size(), GameTitle::MM6);
            if (!staged || staged->needsRecovery)
                throw std::runtime_error("MM6 restart slot did not stage a primary candidate");
            const auto plan = mm6::prepareDetachedOutdoorWorldPlan(
                staged->staged.worldAndVisuals, pTileTable->tileCount(), GameTitle::MM6);
            if (plan.mapName != "oute3.odm")
                throw std::runtime_error("MM6 restart slot selected another first map");

            *pParty = std::move(staged->staged.party);
            *pNPCStats = std::move(staged->staged.worldAndVisuals.world.npcs);
            *gameTimer = std::move(staged->staged.worldAndVisuals.visuals.timer);
            *pActiveOverlayList = std::move(staged->staged.worldAndVisuals.visuals.overlays);
            pMapDeltas = std::move(staged->staged.worldAndVisuals.world.mapDeltas);
            mm6::chestTitleState() = std::move(staged->staged.worldAndVisuals.chestTitle);
            GameUI_LoadPlayerPortraitsAndVoices();
            current_screen_type = SCREEN_GAME;
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            {
                mm6::ScopedChestMapLoadOptIn chestOptIn;
                DoPrepareWorld(/* bLoading = */ true, /* fullscreen = */ 1);
            }
            enteredWorld = engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                uCurrentlyLoadedLevelType == LEVEL_OUTDOOR && !pOutdoor->faces.empty() &&
                !pActors.empty() && vChests.size() == 20 && pMapDeltas.size() == 67 &&
                mm6::chestTitleState().ready && pParty->pos.z == 340;
        } catch (const std::exception &exception) {
            error = exception.what();
        }
        completed = true;
    });
    for (int tick = 0; tick < 256 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    ASSERT_TRUE(error.empty()) << error;
    EXPECT_TRUE(enteredWorld);
    EXPECT_EQ(readFile(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);

    // Exercise the first two control gates of Game::gameLoop after loading,
    // then the movement frame. The parent test loop still belongs to party
    // creation: this is not a production loadGame or gameLoop acceptance test.
    const Vec3f initialPos = pParty->pos;
    const int initialYaw = pParty->_viewYaw;
    bool firstFrameCorrect = false;
    bool secondFrameCorrect = false;
    bool movementFrameCorrect = false;
    std::string stepError;
    game.runGameRoutine([] {
        gameTimer->setPaused(false);
        engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE | ENGINE_SKIP_NEXT_USER_INPUT;
        pPartyActionQueue->Reset();
    });
    for (int phase = 0; phase < 3; ++phase) {
        bool stepped = false;
        game.tick(2);
        game.runGameRoutine([&, phase] {
            try {
                if (phase == 1) {
                    pPartyActionQueue->Add(PARTY_TurnLeft);
                    pPartyActionQueue->Add(PARTY_WalkForward);
                }
                gameTimer->tick();
                current_screen_type = SCREEN_GAME;
                uGameState = GAME_STATE_PLAYING;
                onTimer();
                if (engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) {
                    engineFlags &= ~ENGINE_SKIP_NEXT_WORLD_UPDATE;
                } else {
                    Actor::UpdateActorAI();
                    UpdateUserInput_and_MapSpecificStuff();
                }
                engine->Draw();
                const bool sameWorld =
                    engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                    !engine->_pendingTransition.has_value();
                if (phase == 0) {
                    firstFrameCorrect = sameWorld && pParty->pos == initialPos &&
                        pParty->_viewYaw == initialYaw &&
                        !(engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) &&
                        (engineFlags & ENGINE_SKIP_NEXT_USER_INPUT);
                } else if (phase == 1) {
                    secondFrameCorrect = sameWorld && pParty->pos == initialPos &&
                        pParty->_viewYaw == initialYaw &&
                        !(engineFlags & ENGINE_SKIP_NEXT_USER_INPUT) &&
                        pPartyActionQueue->uNumActions == 2;
                } else {
                    movementFrameCorrect = sameWorld && pParty->pos != initialPos &&
                        pParty->_viewYaw != initialYaw &&
                        pPartyActionQueue->uNumActions == 0;
                }
            } catch (const std::exception &exception) {
                stepError = exception.what();
            }
            stepped = true;
        });
        for (int tick = 0; tick < 256 && !stepped; ++tick)
            game.tick();
        ASSERT_TRUE(stepped);
        ASSERT_TRUE(stepError.empty()) << stepError;
    }
    game.runGameRoutine([] { gameTimer->setPaused(true); });
    ASSERT_TRUE(stepError.empty()) << stepError;
    EXPECT_TRUE(firstFrameCorrect);
    EXPECT_TRUE(secondFrameCorrect);
    EXPECT_TRUE(movementFrameCorrect);
    EXPECT_EQ(readFile(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// Run alone after DISABLED_NewGameControlFlowReachesFirstFrame wrote the slot
// into the same disposable profile. This enters Game::gameLoop through the real
// Main Menu QuickLoad FSM edge, with only its MM6 slot/load body overridden.
// Normal MM6 slot listing and production loadGame remain guarded.
GAME_TEST(MM6ZSaveHeadless, DISABLED_RestartSlotThroughMainMenuGameLoop) {
    const auto profileRoot = restartProfileRoot();
    if (!profileRoot)
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a disposable system-temp directory";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    const std::filesystem::path primary = *profileRoot / "saves" / "autosave.ec6";
    ASSERT_TRUE(std::filesystem::is_regular_file(primary));
    const auto before = readFile(primary);

    bool loadCalled = false;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view fileName) {
            if (fileName != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 test load selected another slot");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            Party freshParty;
            NPCStats freshNpcs = *pNPCStats;
            Timer freshTimer;
            ActiveOverlayList freshOverlays;
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto staged = mm6::stageOfflineSaveSlotForTest(
                *profileRoot, fileName, freshParty, freshNpcs, freshTimer,
                freshOverlays, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, pOverlayTable->overlays.size(), GameTitle::MM6);
            if (!staged || staged->needsRecovery || staged->bytes != before)
                throw std::runtime_error("MM6 test load did not stage the primary slot");
            const auto plan = mm6::prepareDetachedOutdoorWorldPlan(
                staged->staged.worldAndVisuals, pTileTable->tileCount(), GameTitle::MM6);
            if (plan.mapName != "oute3.odm")
                throw std::runtime_error("MM6 test load selected another first map");

            *pParty = std::move(staged->staged.party);
            *pNPCStats = std::move(staged->staged.worldAndVisuals.world.npcs);
            *gameTimer = std::move(staged->staged.worldAndVisuals.visuals.timer);
            *pActiveOverlayList = std::move(staged->staged.worldAndVisuals.visuals.overlays);
            pMapDeltas = std::move(staged->staged.worldAndVisuals.world.mapDeltas);
            mm6::chestTitleState() = std::move(staged->staged.worldAndVisuals.chestTitle);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
            loadCalled = true;
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });

    game.skipLoadingScreen();
    game.tick(3);
    const bool entered = loadCalled && GetCurrentMenuID() == MENU_NONE &&
        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
        uCurrentlyLoadedLevelType == LEVEL_OUTDOOR && !pOutdoor->faces.empty() &&
        !pActors.empty() && vChests.size() == 20 &&
        !(engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) &&
        !(engineFlags & ENGINE_SKIP_NEXT_USER_INPUT) &&
        !engine->_pendingTransition.has_value();
    const Vec3f initialPos = pParty->pos;
    const int initialYaw = pParty->_viewYaw;
    game.runGameRoutine([] {
        pPartyActionQueue->Add(PARTY_TurnLeft);
        pPartyActionQueue->Add(PARTY_WalkForward);
    });
    game.tick(3);
    const bool moved = pParty->pos != initialPos && pParty->_viewYaw != initialYaw &&
        pPartyActionQueue->uNumActions == 0 &&
        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm");

    game.runGameRoutine([&] {
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
        mm6::testing::setLoadGameOverride({});
        chestOptIn.reset();
    });
    EXPECT_TRUE(entered);
    EXPECT_TRUE(moved);
    EXPECT_EQ(readFile(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
