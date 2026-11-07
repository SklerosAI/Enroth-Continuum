#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6CreationItemSeed.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawNewGameSaveRuntime.h"
#include "Engine/MM6/MM6RawChildPublicationForTest.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "GUI/GUIWindow.h"

#include "Utility/String/Format.h"

namespace {

std::optional<std::filesystem::path> disposableV2RestartProfile(bool requireEmpty = true) {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return std::nullopt;
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 v2 writer requires an existing absolute temp profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-"))
        throw std::runtime_error("MM6 v2 writer requires a direct system-temp restart profile");
    constexpr auto otherPermissions = std::filesystem::perms::group_all |
                                      std::filesystem::perms::others_all;
    if ((std::filesystem::status(root).permissions() & otherPermissions) !=
        std::filesystem::perms::none)
        throw std::runtime_error("MM6 v2 writer requires an owner-only profile");
    if (requireEmpty && (std::filesystem::exists(root / "saves" / "autosave.ec6") ||
                         std::filesystem::exists(root / "saves" / "autosave.ec6.bak")))
        throw std::runtime_error("MM6 v2 writer requires an empty restart slot");
    return root;
}

std::vector<std::uint8_t> readDisposableSlot(const std::filesystem::path &profile) {
    std::ifstream input(profile / "saves" / "autosave.ec6", std::ios::binary);
    if (!input)
        throw std::runtime_error("MM6 v2 restart slot is missing");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void completeEditableMM6Party() {
    for (Character &character : pParty->pCharacters) {
        for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
            const Skill skill = character.GetSkillIdxByOrder(order);
            if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                character.pActiveSkills[skill] = CombinedSkillValue::novice();
        }
        if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
            throw std::runtime_error("MM6 creation party lacks four skills");
    }
    for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
        for (Character &character : pParty->pCharacters)
            for (Attribute stat : allStatAttributes())
                character.IncreaseAttribute(stat);
    if (CharacterCreation_GetUnspentAttributePointCount() != 0)
        throw std::runtime_error("MM6 creation party could not spend attribute points");
}

} // namespace

// Run alone in a fresh MM6 headless process. The test-only save callback sees
// the real post-creation Party before first-map entry but writes no save slot.
GAME_TEST(MM6ZCreationItemSeedHeadless, DISABLED_RealStarterItemsStageWithoutDisk) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    EXPECT_FALSE(ufs->exists("saves"));
    test.prepareForNextTest(10, RANDOM_ENGINE_SEQUENTIAL);

    struct Observation {
        int total = 0;
        int plain = 0;
        int standard = 0;
        int antique = 0;
        int rejectedSpecial = 0;
        int rejectedPotion = 0;
        int rejectedRarity = 0;
        int rejectedOther = 0;
        bool called = false;
        bool fullPartyAdmitted = false;
        std::string error;
    } observed;

    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([&] {
        observed.called = true;
        try {
            observed.fullPartyAdmitted = static_cast<bool>(
                mm6::seedMM6CreationItems(*pParty, *pItemTable, GameTitle::MM6));
            for (std::size_t character = 0; character < pParty->pCharacters.size(); ++character) {
                for (InventoryConstEntry entry : pParty->pCharacters[character].inventory.entries()) {
                    ++observed.total;
                    Party single = *pParty;
                    for (Character &member : single.pCharacters)
                        member.inventory = CharacterInventory::forMM6();
                    bool restored = false;
                    if (entry.zone() == INVENTORY_ZONE_GRID) {
                        const Recti rect = entry.geometry();
                        restored = static_cast<bool>(single.pCharacters[character].inventory.tryRestoreAt(
                            entry.index(), Pointi(rect.x, rect.y), *entry));
                    }
                    const bool accepted = restored && static_cast<bool>(
                        mm6::seedMM6CreationItems(single, *pItemTable, GameTitle::MM6));
                    const Item &item = *entry;
                    if (accepted) {
                        if (item.standardEnchantment)
                            ++observed.standard;
                        else if (item.specialEnchantment == ITEM_ENCHANTMENT_ANTIQUE)
                            ++observed.antique;
                        else
                            ++observed.plain;
                    } else if (item.specialEnchantment != ITEM_ENCHANTMENT_NULL) {
                        ++observed.rejectedSpecial;
                    } else if (item.isPotion() || item.potionPower) {
                        ++observed.rejectedPotion;
                    } else if (item.rarity() != RARITY_COMMON) {
                        ++observed.rejectedRarity;
                    } else {
                        ++observed.rejectedOther;
                    }
                }
            }
            // The established detached first-map preflight supplies only the
            // process map/chest cache needed to leave the creation screen.
            // Its disk publication function is deliberately not called.
            auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
            pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
            mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
        } catch (const std::exception &exception) {
            observed.error = exception.what();
        }
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});

    EXPECT_TRUE(observed.called);
    EXPECT_TRUE(observed.error.empty()) << observed.error;
    EXPECT_GT(observed.total, 0);
    EXPECT_EQ(observed.total, observed.plain + observed.standard + observed.antique +
                              observed.rejectedSpecial + observed.rejectedPotion +
                              observed.rejectedRarity + observed.rejectedOther);
    EXPECT_EQ(observed.fullPartyAdmitted,
              observed.rejectedSpecial + observed.rejectedPotion +
              observed.rejectedRarity + observed.rejectedOther == 0);
    EXPECT_FALSE(ufs->exists("saves"));
    EXPECT_EQ(window->nativeHandle(), nullptr);
    fmt::print(stdout,
        "MM6 creation seed counts: total={} plain={} standard={} antique={} "
        "rejectedSpecial={} rejectedPotion={} rejectedRarity={} rejectedOther={} full={}\n",
        observed.total, observed.plain, observed.standard, observed.antique,
        observed.rejectedSpecial, observed.rejectedPotion, observed.rejectedRarity,
        observed.rejectedOther, observed.fullPartyAdmitted);
}

// Run alone, then run MM6ZSaveHeadless.DISABLED_RawV2RestartSlotStagesDetachedSession
// in a second process with the same owner-only system-temp profile. This is
// an explicit test-only disk path; normal MM6 SaveLoad stays guarded.
GAME_TEST(MM6ZCreationItemSeedHeadless, DISABLED_WriteRealStarterV2RestartSlot) {
    const auto profile = disposableV2RestartProfile();
    if (!profile)
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a disposable owner-only system-temp directory";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    EXPECT_FALSE(ufs->exists("saves"));
    test.prepareForNextTest(10, RANDOM_ENGINE_SEQUENTIAL);

    bool wrote = false;
    std::string error;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([&] {
        try {
            const auto seeded = mm6::seedMM6CreationItems(
                *pParty, *pItemTable, GameTitle::MM6);
            if (!seeded)
                throw std::runtime_error("MM6 creation item seed rejected the complete Party");
            mm6::ChestTitleState firstChest;
            if (!mm6::initializePortNewGameChestTitleState(GameTitle::MM6, &firstChest))
                throw std::runtime_error("MM6 v2 restart lacks its chest entropy seed");
            mm6::KnownSaveFields known = mm6::captureFirstMapKnownSeed(
                *pParty, *pNPCStats, *gameTimer, *pActiveOverlayList, "New game",
                pOverlayTable->overlays.size(), GameTitle::MM6);
            known.chest.lcgState = firstChest.lcgState;
            known.chest.generatedArtifactFlags = firstChest.generatedArtifactFlags;
            const mm6::ShopTitleState shops =
                mm6::emptyNewGameShopTitleState(GameTitle::MM6);
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            mm6::PreparedRawNewGameAutosave prepared =
                mm6::prepareOfflineRawNewGameAutosave(
                    known, seeded->party, seeded->inventory, seeded->cursor,
                    shops, {}, maps.expectedNames(),
                    [&maps](std::string_view name) { return maps.readDefaultDelta(name); },
                    [&maps](std::string_view name) { return maps.readBase(name); },
                    verifyItem, *pItemTable, GameTitle::MM6);
            mm6::writeOfflineRawNewGameAutosave(*profile, prepared, GameTitle::MM6);
            wrote = true;

            // Only the first-map cache needed by this disposable game routine
            // is published. The persisted v2 Party and shops stay detached.
            std::unordered_map<std::string, Blob> mapDeltas;
            for (const mm6::SavedMapDelta &delta : prepared.staged.saved.maps.deltas)
                mapDeltas.emplace(delta.name,
                    Blob::copy(delta.bytes.data(), delta.bytes.size()));
            pMapDeltas.swap(mapDeltas);
            mm6::chestTitleState() = firstChest;
        } catch (const std::exception &exception) {
            error = exception.what();
        }
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});

    ASSERT_TRUE(error.empty()) << error;
    ASSERT_TRUE(wrote);
    EXPECT_TRUE(std::filesystem::is_regular_file(*profile / "saves" / "autosave.ec6"));
    EXPECT_FALSE(std::filesystem::exists(*profile / "saves" / "autosave.ec6.bak"));
    EXPECT_FALSE(ufs->exists("saves"));
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// This is the callback that a future disposable Main Menu QuickLoad route may
// install. It is exercised directly and caught here: Game::gameLoop currently
// continues unconditionally after a successful void load override, so a live
// QuickLoad event must remain closed until complete v2 publication exists.
GAME_TEST(MM6ZCreationItemSeedHeadless, DISABLED_V2QuickLoadCallbackRejectsBeforePublish) {
    const auto profile = disposableV2RestartProfile(false);
    if (!profile)
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a populated owner-only system-temp directory";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto beforeSlot = readDisposableSlot(*profile);
    ASSERT_FALSE(beforeSlot.empty());

    bool callbackCalled = false;
    bool exactV2Admitted = false;
    bool publicationRejected = false;
    bool blockersPresent = false;
    bool normalGuardClosed = false;
    bool liveUnchanged = false;
    std::string error;
    game.runGameRoutine([&] {
        const Party beforeParty = *pParty;
        const auto beforeCursor = mm6::rawHeldCursorState(GameTitle::MM6);
        const auto beforeShops = mm6::shopTitleState();
        const auto beforeChest = mm6::chestTitleState();
        const std::size_t beforeMapCount = pMapDeltas.size();
        mm6::testing::setLoadGameOverride([&](std::string_view fileName) {
            callbackCalled = true;
            if (fileName != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 v2 callback selected another slot");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto staged = mm6::preflightStagedRawSessionFields(
                *profile, fileName, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), GameTitle::MM6);
            if (!staged || staged->needsRecovery || !staged->outdoorPlan ||
                staged->indoorPlan || staged->outdoorPlan->mapName != "oute3.odm" ||
                mm6::encodeRawSave(staged->raw.saved, GameTitle::MM6) != beforeSlot)
                throw std::runtime_error("MM6 v2 callback did not admit the exact primary slot");
            exactV2Admitted = true;

            const auto admission = mm6::testing::inspectRawChildPublication(
                *staged, *pItemTable, GameTitle::MM6);
            blockersPresent = std::find(admission.blockers.begin(), admission.blockers.end(),
                mm6::testing::RawChildPublicationBlocker::MouseAssetNotResolved) !=
                    admission.blockers.end() &&
                std::find(admission.blockers.begin(), admission.blockers.end(),
                    mm6::testing::RawChildPublicationBlocker::ActiveWorldResourcesNotBound) !=
                    admission.blockers.end();
            if (admission.canEnterGameLoop())
                throw std::runtime_error("MM6 v2 callback unexpectedly admitted live entry");
            // No live assignment occurs. A caught refusal also prevents the
            // caller from ever returning to Game::gameLoop in this test.
            throw std::runtime_error("MM6 v2 publication gate remains closed");
        });
        try {
            loadGame(mm6::NEW_GAME_AUTOSAVE_SLOT);
        } catch (const std::exception &exception) {
            publicationRejected = std::string_view(exception.what()) ==
                "MM6 v2 publication gate remains closed";
            if (!publicationRejected)
                error = exception.what();
        }
        mm6::testing::setLoadGameOverride({});
        try {
            loadGame(mm6::NEW_GAME_AUTOSAVE_SLOT);
        } catch (const std::runtime_error &exception) {
            normalGuardClosed = std::string_view(exception.what()) ==
                "MM6 save loading is unavailable until the EC6 transaction is complete";
        }
        const auto &afterChest = mm6::chestTitleState();
        bool partyUnchanged = pParty->pos == beforeParty.pos &&
            pParty->GetGold() == beforeParty.GetGold() &&
            pParty->GetFood() == beforeParty.GetFood() &&
            pParty->pPickedItem.itemId == beforeParty.pPickedItem.itemId;
        for (std::size_t character = 0; character < pParty->pCharacters.size(); ++character) {
            const Character &before = beforeParty.pCharacters[character];
            const Character &after = pParty->pCharacters[character];
            partyUnchanged = partyUnchanged && before.name == after.name &&
                before.classType == after.classType && before.uLevel == after.uLevel &&
                before.inventory.size() == after.inventory.size();
        }
        liveUnchanged = partyUnchanged &&
            mm6::rawHeldCursorState(GameTitle::MM6) == beforeCursor &&
            mm6::shopTitleState() == beforeShops &&
            afterChest.ready == beforeChest.ready &&
            afterChest.lcgState == beforeChest.lcgState &&
            afterChest.generatedArtifactFlags == beforeChest.generatedArtifactFlags &&
            pMapDeltas.size() == beforeMapCount;
    });

    EXPECT_TRUE(error.empty()) << error;
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(exactV2Admitted);
    EXPECT_TRUE(publicationRejected);
    EXPECT_TRUE(blockersPresent);
    EXPECT_TRUE(normalGuardClosed);
    EXPECT_TRUE(liveUnchanged);
    EXPECT_EQ(readDisposableSlot(*profile), beforeSlot);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
