#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/MM6PaperdollArt.h"
#include "GUI/UI/UICharacter.h"

#include "Utility/String/Ascii.h"

static void completeEditableMM6Party() {
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

static void enterFirstMapFromNewGame(EngineController &game) {
    // Default party through the creation screen, then the first map in the real game loop. The test-only new-game
    // callback seeds the map cache from the loaded archives and writes no save.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
        pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
        mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void clickCharacterScreenButton(EngineController &game, GUIButton *button) {
    game.pressAndReleaseButton(BUTTON_LEFT, button->rect.topLeft() + Pointi(56, 16)); // The right side of the proposed button rectangle.
    game.tick();
}

GAME_TEST(MM6CharacterScreenHeadless, ButtonArtExistsInMM6Archive) {
    for (std::string_view name : std::array<std::string_view, 14>{
             "buttsta1", "buttsta2", "stat_hot", "buttski1", "buttski2", "skil_hot",
             "buttinv1", "buttinv2", "equi_hot", "buttawa1", "buttawa2", "awar_hot",
             "buttexi1", "buttexi2"}) {
        SCOPED_TRACE(name);
        Sizei size = engine->resources()->iconSize(name);
        EXPECT_GT(size.w, 0);
        EXPECT_GT(size.h, 0);
    }
}

GAME_TEST(MM6CharacterScreenHeadless, PaperdollBaseArtExistsInMM6Archive) {
    const Sizei background = engine->resources()->iconSize("BACKDOLL");
    ASSERT_GT(background.w, 0);
    ASSERT_GT(background.h, 0);
    for (int face = 0; face < mm6::kPortraitFaceCount; ++face) {
        SCOPED_TRACE(face);
        const auto art = mm6::paperdollBaseArt(face);
        ASSERT_TRUE(art.has_value());
        for (const auto &[name, offset] : {std::pair{art->body, art->bodyOffset},
                                          std::pair{art->arm1, art->arm1Offset},
                                          std::pair{art->arm2, art->arm2Offset}}) {
            const Sizei size = engine->resources()->iconSize(name);
            EXPECT_GT(size.w, 0) << name;
            EXPECT_GT(size.h, 0) << name;
            EXPECT_GE(offset.x, 0);
            EXPECT_GE(offset.y, 0);
            EXPECT_LE(offset.x + size.w, background.w) << name;
            EXPECT_LE(offset.y + size.h, background.h) << name;
        }
    }
    for (std::string_view name : {"LEFTHAND", "RTHAND"}) {
        const Sizei size = engine->resources()->iconSize(name);
        EXPECT_GT(size.w, 0);
        EXPECT_GT(size.h, 0);
    }
}

GAME_TEST(MM6CharacterScreenHeadless, ArmorLayersCoverMM6ItemsAndArchive) {
    const Sizei background = engine->resources()->iconSize("BACKDOLL");
    for (const mm6::PaperdollArmorArt &art : mm6::kPaperdollArmorArt) {
        SCOPED_TRACE(art.itemPicture);
        for (const mm6::PaperdollLayer &layer : {art.body, art.arm1, art.arm2}) {
            const Sizei size = engine->resources()->iconSize(layer.image);
            EXPECT_GT(size.w, 0);
            EXPECT_GT(size.h, 0);
            EXPECT_GE(layer.offset.x, 0);
            EXPECT_GE(layer.offset.y, 0);
            EXPECT_LE(layer.offset.x + size.w, background.w);
            EXPECT_LE(layer.offset.y + size.h, background.h);
        }
    }

    int armorCount = 0;
    for (ItemId id : pItemTable->items.indices()) {
        const ItemData &item = pItemTable->items[id];
        if (item.type != ITEM_TYPE_ARMOUR || item.iconName.empty())
            continue;
        ++armorCount;
        EXPECT_NE(mm6::paperdollArmorArt(item.iconName), nullptr) << std::to_underlying(id);
    }
    EXPECT_GT(armorCount, 0);
}

GAME_TEST(MM6CharacterScreenHeadless, WeaponPositionsFitMM6Paperdoll) {
    int itemCount = 0;
    for (ItemId id : pItemTable->items.indices()) {
        const ItemData &item = pItemTable->items[id];
        if (item.iconName.empty() ||
            (item.type != ITEM_TYPE_SINGLE_HANDED && item.type != ITEM_TYPE_TWO_HANDED &&
             item.type != ITEM_TYPE_BOW && item.type != ITEM_TYPE_SHIELD && item.type != ITEM_TYPE_WAND))
            continue;
        ++itemCount;
        SCOPED_TRACE(std::to_underlying(id));
        EXPECT_GE(item.paperdollAnchorOffset.x, 468);
        EXPECT_LT(item.paperdollAnchorOffset.x, 640);
        EXPECT_GE(item.paperdollAnchorOffset.y, 0);
        EXPECT_LT(item.paperdollAnchorOffset.y, 345);
        const Sizei size = engine->resources()->iconSize(item.iconName);
        EXPECT_GT(size.w, 0);
        EXPECT_GT(size.h, 0);
        if (item.type == ITEM_TYPE_SINGLE_HANDED && (item.skill == SKILL_DAGGER || item.skill == SKILL_SWORD)) {
            bool hasPublicOffhandOffset = false;
            for (const mm6::PaperdollOffhandArt &art : mm6::kPaperdollOffhandArt)
                hasPublicOffhandOffset |= ascii::noCaseEquals(art.itemPicture, item.iconName);
            EXPECT_TRUE(hasPublicOffhandOffset || ascii::noCaseEquals(item.iconName, "dagger1") ||
                        ascii::noCaseEquals(item.iconName, "dagger2"));
            const Pointi offhand = mm6::paperdollOffhandPosition(item.iconName, item.paperdollAnchorOffset);
            EXPECT_GE(offhand.x, 468);
            EXPECT_LT(offhand.x, 640);
            EXPECT_GE(offhand.y, 0);
            EXPECT_LT(offhand.y, 345);
        }
    }
    EXPECT_GT(itemCount, 0);
    EXPECT_EQ(mm6::paperdollOffhandPosition("ldagger3", {504, 60}), Pointi(596, 86));
    EXPECT_EQ(mm6::paperdollOffhandPosition("brsword3", {503, 1}), Pointi(596, 28));
    EXPECT_EQ(mm6::paperdollOffhandPosition("lsword5", {502, 8}), Pointi(595, 33));
    EXPECT_EQ(mm6::paperdollOffhandPosition("dagger1", {504, 81}), Pointi(595, 107));
}

GAME_TEST(MM6CharacterScreenHeadless, WearableImagesCoverMM6ItemsAndArchive) {
    int bootsCount = 0;
    int helmsCount = 0;
    int beltsCount = 0;
    for (ItemId id : pItemTable->items.indices()) {
        const ItemData &item = pItemTable->items[id];
        if (item.iconName.empty())
            continue;
        std::string_view image;
        if (item.type == ITEM_TYPE_BOOTS) {
            ++bootsCount;
            image = item.iconName;
        } else if (item.type == ITEM_TYPE_HELMET) {
            ++helmsCount;
            image = mm6::paperdollEquippedImage(item.iconName, mm6::kPaperdollHelmArt);
        } else if (item.type == ITEM_TYPE_BELT) {
            ++beltsCount;
            image = mm6::paperdollEquippedImage(item.iconName, mm6::kPaperdollBeltArt);
        } else {
            continue;
        }
        SCOPED_TRACE(std::to_underlying(id));
        ASSERT_FALSE(image.empty());
        const Sizei size = engine->resources()->iconSize(image);
        EXPECT_GT(size.w, 0);
        EXPECT_GT(size.h, 0);
        EXPECT_GE(item.paperdollAnchorOffset.x, 468);
        EXPECT_LT(item.paperdollAnchorOffset.x, 640);
        EXPECT_GE(item.paperdollAnchorOffset.y, 0);
        EXPECT_LT(item.paperdollAnchorOffset.y, 345);
    }
    EXPECT_GT(bootsCount, 0);
    EXPECT_GT(helmsCount, 0);
    EXPECT_GT(beltsCount, 0);
}

GAME_TEST(MM6CharacterScreenHeadless, TabsOpenByKeyAndButton) {
    // The character screen buttons had MM7 positions and placeholder-sized hit boxes, so clicking the right end of an MM6
    // button picture did nothing.
    // Run alone in a fresh MM6 headless process.
    auto screenTape = tapes.screen();
    auto tabTape = tapes.custom([] { return current_character_screen_window; });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick(2); // The key queues the character screen for the next frame.
    for (PlatformKey key : {PlatformKey::KEY_C, PlatformKey::KEY_S, PlatformKey::KEY_A, PlatformKey::KEY_I}) {
        game.pressAndReleaseKey(key);
        game.tick();
    }
    for (GUIButton *button : {pCharacterScreen_StatsBtn, pCharacterScreen_SkillsBtn, pCharacterScreen_AwardsBtn, pCharacterScreen_InventoryBtn})
        clickCharacterScreenButton(game, button);
    clickCharacterScreenButton(game, pCharacterScreen_ExitBtn);
    game.tick(); // The exit button press draws its pressed picture, then closes the screen on the next frame.

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHARACTERS, SCREEN_GAME));
    EXPECT_EQ(tabTape, tape(WINDOW_CHARACTER_STATS, WINDOW_CHARACTER_INVENTORY,
                            WINDOW_CHARACTER_STATS, WINDOW_CHARACTER_SKILLS, WINDOW_CHARACTER_AWARDS, WINDOW_CHARACTER_INVENTORY,
                            WINDOW_CHARACTER_STATS, WINDOW_CHARACTER_SKILLS, WINDOW_CHARACTER_AWARDS, WINDOW_CHARACTER_INVENTORY));
}

GAME_TEST(MM6CharacterScreenHeadless, EquippedArmorHasPaperdollHitArea) {
    enterFirstMapFromNewGame(game);
    ItemId armorId = ITEM_NULL;
    for (ItemId id : pItemTable->items.indices()) {
        const ItemData &item = pItemTable->items[id];
        if (item.type == ITEM_TYPE_ARMOUR && ascii::noCaseEquals(item.iconName, "lr1icon")) {
            armorId = id;
            break;
        }
    }
    ASSERT_NE(armorId, ITEM_NULL);
    game.runGameRoutine([&] {
        pParty->pCharacters[0].inventory.equip(ITEM_SLOT_ARMOUR, Item(armorId));
    });
    const InventoryEntry armor = pParty->pCharacters[0].inventory.entry(ITEM_SLOT_ARMOUR);
    ASSERT_TRUE(armor);

    game.goToInventory(0);
    game.tick();
    size_t clickablePixels = 0;
    for (int y = 0; y < 300; ++y)
        for (int x = 468; x < 640; ++x)
            clickablePixels += equipmentHitMap.query({x, y}, -1) == armor.index();
    EXPECT_GT(clickablePixels, 100u);
}

GAME_TEST(MM6CharacterScreenHeadless, EquippedArtifactOffhandHasPaperdollHitArea) {
    enterFirstMapFromNewGame(game);
    constexpr ItemId swordId = static_cast<ItemId>(1);
    constexpr ItemId artifactId = static_cast<ItemId>(400);
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[swordId].iconName, "lsword1"));
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[artifactId].iconName, "ldagger3"));
    ASSERT_EQ(pItemTable->items[artifactId].skill, SKILL_DAGGER);
    game.runGameRoutine([&] {
        auto &inventory = pParty->pCharacters[0].inventory;
        for (ItemSlot slot : {ITEM_SLOT_MAIN_HAND, ITEM_SLOT_OFF_HAND})
            if (InventoryEntry old = inventory.entry(slot))
                inventory.take(old);
        inventory.equip(ITEM_SLOT_MAIN_HAND, Item(swordId));
        inventory.equip(ITEM_SLOT_OFF_HAND, Item(artifactId));
    });
    auto &inventory = pParty->pCharacters[0].inventory;
    const InventoryEntry sword = inventory.entry(ITEM_SLOT_MAIN_HAND);
    const InventoryEntry artifact = inventory.entry(ITEM_SLOT_OFF_HAND);
    ASSERT_TRUE(sword);
    ASSERT_TRUE(artifact);

    game.goToInventory(0);
    game.tick();
    size_t swordPixels = 0;
    size_t artifactPixels = 0;
    for (int y = 0; y < 300; ++y)
        for (int x = 468; x < 640; ++x) {
            const int hit = equipmentHitMap.query({x, y}, -1);
            swordPixels += hit == sword.index();
            artifactPixels += hit == artifact.index();
        }
    EXPECT_GT(swordPixels, 100u);
    EXPECT_GT(artifactPixels, 100u);
}

GAME_TEST(MM6CharacterScreenHeadless, EquippedBowAndShieldHavePaperdollHitAreas) {
    enterFirstMapFromNewGame(game);
    constexpr ItemId bowId = static_cast<ItemId>(42);
    constexpr ItemId shieldId = static_cast<ItemId>(79);
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[bowId].iconName, "bow1"));
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[shieldId].iconName, "lshield1"));
    game.runGameRoutine([&] {
        auto &inventory = pParty->pCharacters[0].inventory;
        for (ItemSlot slot : {ITEM_SLOT_BOW, ITEM_SLOT_OFF_HAND})
            if (InventoryEntry old = inventory.entry(slot))
                inventory.take(old);
        inventory.equip(ITEM_SLOT_BOW, Item(bowId));
        inventory.equip(ITEM_SLOT_OFF_HAND, Item(shieldId));
    });
    auto &inventory = pParty->pCharacters[0].inventory;
    const InventoryEntry bow = inventory.entry(ITEM_SLOT_BOW);
    const InventoryEntry shield = inventory.entry(ITEM_SLOT_OFF_HAND);
    ASSERT_TRUE(bow);
    ASSERT_TRUE(shield);

    game.goToInventory(0);
    game.tick();
    size_t bowPixels = 0;
    size_t shieldPixels = 0;
    for (int y = 0; y < 300; ++y)
        for (int x = 468; x < 640; ++x) {
            const int hit = equipmentHitMap.query({x, y}, -1);
            bowPixels += hit == bow.index();
            shieldPixels += hit == shield.index();
        }
    EXPECT_GT(bowPixels, 100u);
    EXPECT_GT(shieldPixels, 100u);
}

GAME_TEST(MM6CharacterScreenHeadless, EquippedBootsHelmAndBeltHavePaperdollHitAreas) {
    enterFirstMapFromNewGame(game);
    constexpr ItemId helmId = static_cast<ItemId>(89);
    constexpr ItemId beltId = static_cast<ItemId>(100);
    constexpr ItemId bootsId = static_cast<ItemId>(115);
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[helmId].iconName, "helm1"));
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[beltId].iconName, "belt1a"));
    ASSERT_TRUE(ascii::noCaseEquals(pItemTable->items[bootsId].iconName, "boots1"));
    game.runGameRoutine([&] {
        auto &inventory = pParty->pCharacters[0].inventory;
        for (ItemSlot slot : {ITEM_SLOT_HELMET, ITEM_SLOT_BELT, ITEM_SLOT_BOOTS})
            if (InventoryEntry old = inventory.entry(slot))
                inventory.take(old);
        inventory.equip(ITEM_SLOT_HELMET, Item(helmId));
        inventory.equip(ITEM_SLOT_BELT, Item(beltId));
        inventory.equip(ITEM_SLOT_BOOTS, Item(bootsId));
    });
    auto &inventory = pParty->pCharacters[0].inventory;
    const InventoryEntry helm = inventory.entry(ITEM_SLOT_HELMET);
    const InventoryEntry belt = inventory.entry(ITEM_SLOT_BELT);
    const InventoryEntry boots = inventory.entry(ITEM_SLOT_BOOTS);
    ASSERT_TRUE(helm);
    ASSERT_TRUE(belt);
    ASSERT_TRUE(boots);

    game.goToInventory(0);
    game.tick();
    size_t helmPixels = 0;
    size_t beltPixels = 0;
    size_t bootsPixels = 0;
    for (int y = 0; y < 345; ++y)
        for (int x = 468; x < 640; ++x) {
            const int hit = equipmentHitMap.query({x, y}, -1);
            helmPixels += hit == helm.index();
            beltPixels += hit == belt.index();
            bootsPixels += hit == boots.index();
        }
    EXPECT_GT(helmPixels, 100u);
    EXPECT_GT(beltPixels, 100u);
    EXPECT_GT(bootsPixels, 100u);
}

GAME_TEST(MM6CharacterScreenHeadless, TabKeyRightAfterInventoryKey) {
    // Pressing C on the frame after I, before the character screen opened, crashed on a freed tab button.
    // Run alone in a fresh MM6 headless process.
    auto screenTape = tapes.screen();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_SPELL_BOOK));
}
