#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6KnownSaveFields.h"
#include "Engine/MM6/MM6NewGameSaveRuntime.h"
#include "Engine/MM6/MM6PrivateMapSource.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/OverlayTable.h"

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

} // namespace

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
        bool freshDiskStage = false;
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
    EXPECT_TRUE(result.wrongTitleRejected);
    EXPECT_TRUE(result.corruptPrimaryRecoveredReadOnly);
    EXPECT_FALSE(result.primaryBeforeFailure.empty());
    EXPECT_FALSE(result.backupBeforeFailure.empty());
    EXPECT_EQ(mm6::decodeKnownSave(result.backupBeforeFailure).chest.lcgState, result.firstSeed);
    EXPECT_EQ(mm6::decodeKnownSave(result.primaryBeforeFailure).chest.lcgState, result.secondSeed);
    EXPECT_EQ(result.primaryAfterFailure, result.primaryBeforeFailure);
    EXPECT_EQ(result.backupAfterFailure, result.backupBeforeFailure);
    EXPECT_TRUE(result.rollbackExact);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
