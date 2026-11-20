#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6ArtifactCollection.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/UI/UIRest.h"
#include "GUI/UI/UISaveLoad.h"

#include "Library/Environment/Interface/Environment.h"

#include "MM6CampaignChain.h"

static constexpr std::uint32_t hiveArtifactLuck = 4; // Chest luck that rolls artifact 415 into the hive's chest 4 at tier 6, and no other artifact.
static constexpr ItemId hiveArtifact = static_cast<ItemId>(415);

struct ScopedAllArtifacts {
    // Sets the MM6 All Artifacts switch the way Atlas does, and clears it when the test is done with it.
    explicit ScopedAllArtifacts(bool on) {
        environment->setenv("ENROTH_MM6_ALL_ARTIFACTS", on ? "1" : "0");
    }
    ~ScopedAllArtifacts() {
        environment->setenv("ENROTH_MM6_ALL_ARTIFACTS", "");
    }
    std::unique_ptr<Environment> environment = Environment::createStandardEnvironment();
};

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 artifact ledger test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_ALL_ARTIFACTS_SAVE_ENABLE");
    return flag && std::string_view(flag) == value;
}

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static std::vector<int> chestArtifacts(int chestId) {
    std::vector<int> result;
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        if (std::to_underlying(entry->itemId) >= 400 && std::to_underlying(entry->itemId) <= 429)
            result.push_back(std::to_underlying(entry->itemId));
    return result;
}

static int generatedArtifacts() {
    return std::ranges::count(mm6::chestTitleState().generatedArtifactFlags, 1);
}

static void restDays(EngineController &game, int days) {
    // Rests and heals once a day in the rest screen, then waits there for the next dawn, that many days.
    for (int day = 0; day < days; day++) {
        for (std::string_view button : {"Rest_RestAndHeal", "Rest_WaitTillDawn"}) {
            game.pressAndReleaseKey(PlatformKey::KEY_R);
            game.tick();
            game.pressGuiButton(button);
            game.tick();
            while (currentRestType != REST_NONE)
                game.tick(); // The hours pass.
            if (current_screen_type == SCREEN_REST) {
                game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
                game.tick();
            }
        }
    }
}

static void openHiveChest4(EngineController &game) {
    // The party arrives next to the hive's chest 4 with the chest luck that rolls artifact 415 there, and opens it.
    mm6::chestTitleState().lcgState = hiveArtifactLuck;
    game.teleportTo(pMapTable->GetMapInfo("hive.blv"), Vec3f(-3000, 4182, -1777), 0, -20); // West of chest 4, looking down at it.
    game.pointMouseAtFace(2960);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static void waitForHiveRespawn(EngineController &game) {
    // The party rests a week in the Oracle, where no random encounters come, for the hive to refill.
    game.teleportTo(pMapTable->GetMapInfo("oracle.blv"), Vec3f(-1408, 900, -512), 90);
    pParty->SetFood(14); // Seven rests at two food each, a new party has seven.
    restDays(game, 7);
}

GAME_TEST(MM6AllArtifacts, ArtifactRollsPastTheCap) {
    // MM6 rolls no random artifact once 13 have been generated. With the All Artifacts mod a tier-6 roll still gives a
    // new one, and without it the cap holds.
    for (bool modOn : {true, false}) {
        SCOPED_TRACE(fmt::format("modOn={}", modOn));
        test.prepareForNextTest();
        ScopedAllArtifacts mod(modOn);
        const MapId hive = pMapTable->GetMapInfo("hive.blv");
        auto artifactTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestArtifacts(4)) : std::nullopt; });
        auto generatedTape = tapes.custom([] { return generatedArtifacts(); });
        auto screenTape = tapes.screen();
        startNewGameThroughInput(game);
        test.startTaping();

        for (int index = 0; index < 13; index++)
            mm6::chestTitleState().generatedArtifactFlags[index] = 1; // Artifacts 400 to 412 were rolled elsewhere.
        openHiveChest4(game);

        EXPECT_EQ(screenTape.back(), SCREEN_CHEST);
        EXPECT_EQ(artifactTape.back(), modOn ? std::vector{415} : std::vector<int>());
        EXPECT_EQ(generatedTape, modOn ? tape(13, 14) : tape(13));
    }
}

GAME_TEST(MM6AllArtifacts, ArtifactComesBackUntilCollected) {
    // With the All Artifacts mod an artifact the party saw in a chest and left there rolls again once the map refills,
    // and one the party took never does. Without the mod the artifact left in the chest is gone for good.
    struct Case {
        bool modOn;
        bool take;
        bool comesBack;
    };
    for (Case c : {Case{true, false, true}, Case{true, true, false}, Case{false, false, false}}) {
        SCOPED_TRACE(fmt::format("modOn={} take={}", c.modOn, c.take));
        test.prepareForNextTest();
        ScopedAllArtifacts mod(c.modOn);
        const MapId hive = pMapTable->GetMapInfo("hive.blv");
        auto artifactTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestArtifacts(4)) : std::nullopt; });
        auto respawnTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(pIndoor->dlv.respawnCount) : std::nullopt; });
        auto carriedTape = charTapes.hasItem(0, hiveArtifact);
        auto collectedTape = tapes.custom([] { return mm6::artifactCollection().collected[415 - 400]; });
        startNewGameThroughInput(game);
        test.startTaping();

        openHiveChest4(game);
        if (c.take) {
            takeMM6CampaignChainChestItem(game, 4, hiveArtifact);
        } else {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
        waitForHiveRespawn(game);
        openHiveChest4(game);

        EXPECT_EQ(respawnTape, tape(std::optional<int>(), 0, std::nullopt, 1));
        EXPECT_EQ(artifactTape.back(), c.comesBack ? std::vector{415} : std::vector<int>());
        EXPECT_CONTAINS(artifactTape, std::optional(std::vector{415}));
        EXPECT_EQ(carriedTape, c.take ? tape(false, true) : tape(false));
        EXPECT_EQ(collectedTape, c.modOn && c.take ? tape(0, 1) : tape(0));
    }
}

GAME_TEST(MM6AllArtifactsHeadless, DISABLED_LedgerSavesWithTheParty) {
    // With the All Artifacts mod a party takes artifact 415 from the hive's chest 4, throws it away, waits a week in the
    // Oracle and saves, the first half of the ledger round trip. A save refuses a party that carries an artifact.
    if (!enabled("strict-artifact-ledger-write"))
        GTEST_SKIP() << "Run only as the artifact ledger write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    ASSERT_FALSE(std::filesystem::exists(slot));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });
    ScopedAllArtifacts mod(true);
    auto statusTape = tapes.statusBar();
    auto heldTape = tapes.custom([] { return std::to_underlying(pParty->pPickedItem.itemId); });
    auto collectedTape = tapes.custom([] { return mm6::artifactCollection().collected[415 - 400]; });
    startNewGameThroughInput(game);
    test.startTaping();

    openHiveChest4(game);
    InventoryEntry artifact = vChests[4].inventory.find(hiveArtifact);
    ASSERT_TRUE(artifact);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[4].chestTypeId].inventoryOffset + artifact.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, 240, 160); // The middle of the 3D view, the party throws the artifact.
    game.tick();
    waitForHiveRespawn(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_A);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.

    EXPECT_EQ(heldTape, tape(0, 415, 0));
    EXPECT_EQ(collectedTape, tape(0, 1));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_TRUE(std::filesystem::is_regular_file(slot));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6AllArtifactsHeadless, DISABLED_LedgerComesBackInFreshProcess) {
    // A fresh MM6 process with the All Artifacts mod loads the slot and still knows the party collected artifact 415, so
    // the refilled hive's chest 4 doesn't roll it again with the luck that rolled it the first time.
    if (!enabled("strict-artifact-ledger-reload"))
        GTEST_SKIP() << "Run only after the artifact ledger write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });
    ScopedAllArtifacts mod(true);
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    auto screenTape = tapes.screen();
    auto artifactTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestArtifacts(4)) : std::nullopt; });
    auto respawnTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(pIndoor->dlv.respawnCount) : std::nullopt; });
    test.startTaping();

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto saved = std::ranges::find(slots, "save000.ec6", [](const auto &entry) { return entry.fileName; });
    ASSERT_NE(saved, slots.end());
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", saved - slots.begin()));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    openHiveChest4(game);

    EXPECT_EQ(screenTape.back(), SCREEN_CHEST);
    EXPECT_EQ(respawnTape.back(), 1);
    EXPECT_EQ(artifactTape.back(), std::vector<int>());
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}
