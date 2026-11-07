#include <memory>
#include <stdexcept>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6CreationItemSeed.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "GUI/GUIWindow.h"

#include "Utility/String/Format.h"

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
        for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
            for (Character &character : pParty->pCharacters)
                for (Attribute stat : allStatAttributes())
                    character.IncreaseAttribute(stat);
        if (CharacterCreation_GetUnspentAttributePointCount() != 0)
            throw std::runtime_error("MM6 creation party could not spend attribute points");
    });
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
