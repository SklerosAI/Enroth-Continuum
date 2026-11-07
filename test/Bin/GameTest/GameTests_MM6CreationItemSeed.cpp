#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6CreationItemSeed.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6RawNewGameSaveRuntime.h"
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
#include "Engine/Timer.h"
#include "GUI/GUIWindow.h"

#include "Utility/String/Format.h"

namespace {

std::optional<std::filesystem::path> disposableV2RestartProfile() {
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
    if (std::filesystem::exists(root / "saves" / "autosave.ec6") ||
        std::filesystem::exists(root / "saves" / "autosave.ec6.bak"))
        throw std::runtime_error("MM6 v2 writer requires an empty restart slot");
    return root;
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
