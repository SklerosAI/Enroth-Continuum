#include <algorithm>
#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/Chest.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"

#include "GUI/GUIWindow.h"
#include "GUI/GUIButton.h"
#include "GUI/UI/MM6CharacterScreen.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/UICharacter.h"
#include "GUI/UI/UIGame.h"

#include "Library/Image/Png.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Ascii.h"
#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

GAME_TEST(MM6SpriteOctants, SevenCharacterDirectionalPrefix) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const int frameId = pSpriteFrameTable->FastFindSprite("pfemsta");
    ASSERT_GT(frameId, 0);
    SpriteFrame *frame = pSpriteFrameTable->GetFrame(frameId, 0_ticks);
    ASSERT_NE(frame, nullptr);
    for (int octant = 0; octant < 8; ++octant)
        EXPECT_NE(frame->sprites[octant], nullptr) << "octant " << octant;
}

GAME_TEST(MM6TerrainTextureUnits, AuthoredOutdoorLandFitsUnitOne) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    size_t maps = 0;
    size_t peakLandLayers = 0;
    const Sizei waterSize = engine->resources()->bitmapSize("wtrtyl");
    ASSERT_EQ(waterSize, Sizei(128, 128));

    for (const std::string &name : pGames_LOD->ls()) {
        if (!name.ends_with(".odm"))
            continue;
        auto base = std::make_unique<OutdoorLocation_MM7>();
        deserializeMM6Outdoor(decodeMM6ArchiveEntry(pGames_LOD->read(name)), base.get());
        auto terrain = std::make_unique<OutdoorTerrain>();
        reconstruct(*base, terrain.get(), GameTitle::MM6);

        for (int month = 0; month < 12; ++month) {
            terrain->changeSeason(month, GameTitle::MM6);
            std::set<std::pair<std::string, bool>> landNames;
            for (int y = 0; y < 127; ++y) {
                for (int x = 0; x < 127; ++x) {
                    const TileData &tile = terrain->tileDataByGrid({x, y});
                    if (tile.textureName == "wtrtyl")
                        continue;
                    landNames.emplace(tile.textureName, tile.flags & TILE_GENERATED_TRANSITION);
                }
            }
            for (const auto &[textureName, generated] : landNames) {
                const Sizei size = generated ? engine->resources()->generatedSize() :
                    engine->resources()->bitmapSize(textureName);
                EXPECT_EQ(size, waterSize) << name << " month " << month;
            }
            peakLandLayers = std::max(peakLandLayers, landNames.size());
            EXPECT_LE(landNames.size(), 256u) << name << " month " << month;
        }
        ++maps;
    }
    EXPECT_EQ(maps, 15u);
    EXPECT_GT(peakLandLayers, 0u);
}

GAME_TEST(MM6SpritePalette, TreeFrameUsesRecordedPalette) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const int frameId = pSpriteFrameTable->FastFindSprite("tree01");
    ASSERT_GT(frameId, 0);
    const SpriteFrame &frame = pSpriteFrameTable->pSpriteSFrames[frameId];
    EXPECT_EQ(frame.paletteId, 140);
    EXPECT_EQ(frame.glowRadius, 0);
    EXPECT_EQ(frame.frameLength, Duration::fromTicks(8));
    ASSERT_NE(pSpriteFrameTable->GetFrame(frameId, 0_ticks)->sprites[0], nullptr);
    const Palette palette = engine->resources()->palette(frame.paletteId);
    size_t green = 0;
    for (const Color &color : palette.colors)
        green += color.g > color.r && color.g > color.b;
    EXPECT_GT(green, 20u);

    const int redTreeId = pSpriteFrameTable->FastFindSprite("tree03");
    ASSERT_GT(redTreeId, 0);
    EXPECT_EQ(pSpriteFrameTable->pSpriteSFrames[redTreeId].paletteId, 140);
    auto dominantPixels = [&](std::string_view name, bool redDominant) {
        const RgbaImage indexed = engine->resources()->sprite(name);
        size_t count = 0;
        for (Color pixel : indexed.pixels()) {
            if (!pixel.a)
                continue;
            const Color mapped = palette.colors[pixel.r];
            count += redDominant ? (mapped.r > mapped.g * 1.3 && mapped.r > mapped.b * 1.3) :
                                   (mapped.g > mapped.r * 1.3 && mapped.g > mapped.b * 1.3);
        }
        return count;
    };
    EXPECT_GT(dominantPixels("tree01", false), 1000u);
    EXPECT_GT(dominantPixels("tree03", true), 1000u);
}

// Explicit, fresh-process, macOS-only capture. --offscreen-gl installs the
// hidden SDL OpenGL input/focus guard before GameStarter configures its window.
GAME_TEST(MM6OffscreenFrame, DISABLED_FirstOutdoorCapture) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(window->nativeHandle(), nullptr);
    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);

    const char *capturePath = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    const std::string_view destination(capturePath);
    ASSERT_TRUE(destination.starts_with("/tmp/") || destination.starts_with("/private/tmp/"));
    ASSERT_TRUE(Path(capturePath).isNormalized());
    ASSERT_EQ(Path(capturePath).extension(), ".png");

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    ASSERT_EQ(lWindowList.size(), 2u);

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    bool loaded = false;
    game.runGameRoutine([firstMap, &loaded] {
        GameUI_LoadPlayerPortraitsAndVoices();
        engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
        {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        }
        loaded = !mm6::chestMapLoadOptedIn();
    });
    for (int tick = 0; tick < 256 && !loaded; ++tick)
        game.tick();
    ASSERT_TRUE(loaded);
    ASSERT_EQ(engine->_currentLoadedMapId, firstMap);
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    ASSERT_EQ(vChests.size(), 20u);
    ASSERT_EQ(lWindowList.size(), 1u);
    ASSERT_FALSE(window->isVisible());

    RgbaImage frame;
    game.runGameRoutine([&frame] {
        uGameState = GAME_STATE_PLAYING;
        current_screen_type = SCREEN_GAME;
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot(); // Read before swap; no display presentation.
    });

    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(frame.size(), Sizei(640, 480));
    const Color first = frame[0][0];
    size_t changed = 0;
    size_t nonblackWorld = 0;
    size_t nonblackHud = 0;
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            Color pixel = frame[y][x];
            changed += pixel != first;
            if (pixel.r || pixel.g || pixel.b) {
                if (x < 460 && y < 350)
                    ++nonblackWorld;
                if (y >= 350)
                    ++nonblackHud;
            }
        }
    }
    EXPECT_GT(changed, 10000u);
    EXPECT_GT(nonblackWorld, 10000u);
    EXPECT_GT(nonblackHud, 10000u);
    // Before the normal 2D flush, these four inner face regions contain only
    // the monochrome frame backing even though portrait quads were queued.
    for (int center : {50, 163, 276, 388}) {
        size_t chromaticPixels = 0;
        for (int y = 405; y < 445; ++y) {
            for (int x = center - 12; x < center + 13; ++x) {
                const Color pixel = frame[y][x];
                const int spread = std::max({pixel.r, pixel.g, pixel.b}) -
                                   std::min({pixel.r, pixel.g, pixel.b});
                chromaticPixels += spread > 30;
            }
        }
        EXPECT_GT(chromaticPixels, 250u) << "portrait center " << center;
    }

    if (const char *check = std::getenv("MM6_OFFSCREEN_SELECTION_ANIMATION_CHECK");
        check && std::string_view(check) == "1") {
        RgbaImage firstSelection;
        RgbaImage secondSelection;
        game.runGameRoutine([&] {
            const Duration priorTime = animTimer->time();
            const bool priorPaused = animTimer->isPaused();
            pParty->setActiveCharacterIndex(0);
            auto captureSelection = [&](Duration time) {
                animTimer->restorePausedSimulationTime(time);
                render->ClearTarget(Color(0, 0, 0));
                render->BeginScene2D();
                GameUI_DrawCharacterSelectionFrame();
                render->flushAndScale();
                return render->MakeFullScreenshot();
            };
            firstSelection = captureSelection(0_ticks);
            secondSelection = captureSelection(8_ticks);
            animTimer->restorePausedSimulationTime(priorTime);
            animTimer->setPaused(priorPaused);
        });
        ASSERT_EQ(firstSelection.size(), frame.size());
        ASSERT_EQ(secondSelection.size(), frame.size());
        size_t changedSelectionPixels = 0;
        for (int y = 377; y < 468; ++y)
            for (int x = 16; x < 85; ++x)
                changedSelectionPixels += firstSelection[y][x] != secondSelection[y][x];
        EXPECT_GT(changedSelectionPixels, 10u);
    }

    FileOutputStream output{Path(capturePath)};
    output.write(png::encode(frame));
    output.close();

    // Optional second private capture of the MM6 first-open chest screen.
    // The hidden test keeps the GUI in scope only while the engine is alive.
    if (const char *chestPath = std::getenv("MM6_OFFSCREEN_CHEST_CAPTURE_PATH")) {
        const std::string_view chestDestination(chestPath);
        ASSERT_TRUE(chestDestination.starts_with("/tmp/") ||
                    chestDestination.starts_with("/private/tmp/"));
        ASSERT_TRUE(Path(chestPath).isNormalized());
        ASSERT_EQ(Path(chestPath).extension(), ".png");
        int chestIndex = -1;
        int emptyFallback = -1;
        for (int index = 0; index < static_cast<int>(vChests.size()); ++index) {
            if (!vChests[index].Trapped() && !vChests[index].Initialized()) {
                if (emptyFallback < 0)
                    emptyFallback = index;
                Chest_MM7 raw{};
                snapshot(vChests[index], &raw);
                if (std::any_of(raw.items.begin(), raw.items.end(), [](const Item_MM7 &item) {
                        return item.itemId > 0;
                    })) {
                    chestIndex = index;
                    break;
                }
            }
        }
        if (chestIndex < 0)
            chestIndex = emptyFallback;
        ASSERT_GE(chestIndex, 0);
        RgbaImage chestFrame;
        bool opened = false;
        game.runGameRoutine([&] {
            try {
                pParty->setActiveCharacterIndex(0);
                opened = Chest::open(chestIndex, Pid::dummy());
                if (opened) {
                    render->ClearTarget(Color(0, 0, 0));
                    engine->drawWorld();
                    engine->drawHUD();
                    render->flushAndScale();
                    engine->drawOverlay();
                    chestFrame = render->MakeFullScreenshot();
                }
            } catch (...) {
                pGUIWindow_CurrentChest.reset();
                current_screen_type = SCREEN_GAME;
                gameTimer->setPaused(false);
                throw;
            }
            pGUIWindow_CurrentChest.reset();
            current_screen_type = SCREEN_GAME;
            gameTimer->setPaused(false);
        });
        ASSERT_TRUE(opened);
        ASSERT_EQ(chestFrame.size(), frame.size());
        size_t changedChestPixels = 0;
        for (int y = 0; y < frame.height(); ++y)
            for (int x = 0; x < frame.width(); ++x)
                changedChestPixels += frame[y][x] != chestFrame[y][x];
        EXPECT_GT(changedChestPixels, 10000u);
        EXPECT_FALSE(window->isVisible());
        EXPECT_FALSE(window->grabsMouse());
        FileOutputStream chestOutput{Path(chestPath)};
        chestOutput.write(png::encode(chestFrame));
        chestOutput.close();
    }

    // Optional diagnostic for the MM6 character record. Compare two tab
    // states in the hidden framebuffer, then keep one local capture for F5a.
    if (const char *characterPath = std::getenv("MM6_OFFSCREEN_CHARACTER_CAPTURE_PATH")) {
        const std::string_view characterDestination(characterPath);
        ASSERT_TRUE(characterDestination.starts_with("/tmp/") ||
                    characterDestination.starts_with("/private/tmp/"));
        ASSERT_TRUE(Path(characterPath).isNormalized());
        ASSERT_EQ(Path(characterPath).extension(), ".png");

        game.runGameRoutine([] {
            pParty->setActiveCharacterIndex(0);
            pGUIWindow_CurrentMenu = std::make_unique<GUIWindow_CharacterRecord>(SCREEN_CHARACTERS);
            static_cast<GUIWindow_CharacterRecord *>(pGUIWindow_CurrentMenu.get())->ShowInventoryTab();
        });
        ASSERT_EQ(current_screen_type, SCREEN_CHARACTERS);
        ASSERT_EQ(current_character_screen_window, WINDOW_CHARACTER_INVENTORY);

        auto captureCharacter = [&] {
            RgbaImage result;
            game.runGameRoutine([&] {
                render->ClearTarget(Color(0, 0, 0));
                engine->drawWorld();
                engine->drawHUD();
                render->flushAndScale();
                engine->drawOverlay();
                result = render->MakeFullScreenshot();
            });
            return result;
        };
        const RgbaImage inventoryFrame = captureCharacter();
        ASSERT_EQ(inventoryFrame.size(), frame.size());
        FileOutputStream characterOutput{Path(characterPath)};
        characterOutput.write(png::encode(inventoryFrame));
        characterOutput.close();

        if (const char *armorPath = std::getenv("MM6_OFFSCREEN_ARMOR_CAPTURE_PATH")) {
            const std::string_view armorDestination(armorPath);
            ASSERT_TRUE(armorDestination.starts_with("/tmp/") ||
                        armorDestination.starts_with("/private/tmp/"));
            ASSERT_TRUE(Path(armorPath).isNormalized());
            ASSERT_EQ(Path(armorPath).extension(), ".png");

            ItemId armorId = ITEM_NULL;
            for (ItemId id : pItemTable->items.indices()) {
                if (ascii::noCaseEquals(pItemTable->items[id].iconName, "lr1icon") &&
                    pItemTable->items[id].type == ITEM_TYPE_ARMOUR) {
                    armorId = id;
                    break;
                }
            }
            ASSERT_NE(armorId, ITEM_NULL);
            game.runGameRoutine([&] {
                pParty->activeCharacter().inventory.equip(ITEM_SLOT_ARMOUR, Item(armorId));
            });
            ASSERT_TRUE(pParty->activeCharacter().inventory.entry(ITEM_SLOT_ARMOUR));
            const RgbaImage armorFrame = captureCharacter();
            ASSERT_EQ(armorFrame.size(), inventoryFrame.size());
            size_t changedPaperdollPixels = 0;
            for (int y = 0; y < 300; ++y)
                for (int x = 468; x < 640; ++x)
                    changedPaperdollPixels += inventoryFrame[y][x] != armorFrame[y][x];
            EXPECT_GT(changedPaperdollPixels, 250u);
            FileOutputStream armorOutput{Path(armorPath)};
            armorOutput.write(png::encode(armorFrame));
            armorOutput.close();
        }

        if (const char *weaponPath = std::getenv("MM6_OFFSCREEN_WEAPON_CAPTURE_PATH")) {
            const std::string_view weaponDestination(weaponPath);
            ASSERT_TRUE(weaponDestination.starts_with("/tmp/") ||
                        weaponDestination.starts_with("/private/tmp/"));
            ASSERT_TRUE(Path(weaponPath).isNormalized());
            ASSERT_EQ(Path(weaponPath).extension(), ".png");

            const RgbaImage beforeWeapons = captureCharacter();
            game.runGameRoutine([] {
                auto &inventory = pParty->activeCharacter().inventory;
                for (ItemSlot slot : {ITEM_SLOT_MAIN_HAND, ITEM_SLOT_OFF_HAND})
                    if (InventoryEntry old = inventory.entry(slot))
                        inventory.take(old);
                inventory.equip(ITEM_SLOT_MAIN_HAND, Item(static_cast<ItemId>(1)));
                inventory.equip(ITEM_SLOT_OFF_HAND, Item(static_cast<ItemId>(400)));
            });
            const RgbaImage weaponFrame = captureCharacter();
            ASSERT_EQ(weaponFrame.size(), beforeWeapons.size());
            size_t changedPaperdollPixels = 0;
            for (int y = 0; y < 300; ++y)
                for (int x = 468; x < 640; ++x)
                    changedPaperdollPixels += beforeWeapons[y][x] != weaponFrame[y][x];
            EXPECT_GT(changedPaperdollPixels, 250u);
            FileOutputStream weaponOutput{Path(weaponPath)};
            weaponOutput.write(png::encode(weaponFrame));
            weaponOutput.close();
        }

        if (const char *wearablePath = std::getenv("MM6_OFFSCREEN_WEARABLE_CAPTURE_PATH")) {
            const std::string_view wearableDestination(wearablePath);
            ASSERT_TRUE(wearableDestination.starts_with("/tmp/") ||
                        wearableDestination.starts_with("/private/tmp/"));
            ASSERT_TRUE(Path(wearablePath).isNormalized());
            ASSERT_EQ(Path(wearablePath).extension(), ".png");

            const RgbaImage beforeWearables = captureCharacter();
            game.runGameRoutine([] {
                auto &inventory = pParty->activeCharacter().inventory;
                for (ItemSlot slot : {ITEM_SLOT_HELMET, ITEM_SLOT_BELT, ITEM_SLOT_BOOTS})
                    if (InventoryEntry old = inventory.entry(slot))
                        inventory.take(old);
                inventory.equip(ITEM_SLOT_HELMET, Item(static_cast<ItemId>(89)));
                inventory.equip(ITEM_SLOT_BELT, Item(static_cast<ItemId>(100)));
                inventory.equip(ITEM_SLOT_BOOTS, Item(static_cast<ItemId>(115)));
            });
            const RgbaImage wearableFrame = captureCharacter();
            ASSERT_EQ(wearableFrame.size(), beforeWearables.size());
            size_t changedPaperdollPixels = 0;
            for (int y = 0; y < 345; ++y)
                for (int x = 468; x < 640; ++x)
                    changedPaperdollPixels += beforeWearables[y][x] != wearableFrame[y][x];
            EXPECT_GT(changedPaperdollPixels, 250u);
            FileOutputStream wearableOutput{Path(wearablePath)};
            wearableOutput.write(png::encode(wearableFrame));
            wearableOutput.close();
        }

        if (const char *cloakPath = std::getenv("MM6_OFFSCREEN_CLOAK_CAPTURE_PATH")) {
            const std::string_view cloakDestination(cloakPath);
            ASSERT_TRUE(cloakDestination.starts_with("/tmp/") ||
                        cloakDestination.starts_with("/private/tmp/"));
            ASSERT_TRUE(Path(cloakPath).isNormalized());
            ASSERT_EQ(Path(cloakPath).extension(), ".png");

            ItemId cloakId = ITEM_NULL;
            for (ItemId id : pItemTable->items.indices()) {
                const ItemData &item = pItemTable->items[id];
                if (item.type == ITEM_TYPE_CLOAK && ascii::noCaseEquals(item.iconName, "cape1a")) {
                    cloakId = id;
                    break;
                }
            }
            ASSERT_NE(cloakId, ITEM_NULL);
            const RgbaImage beforeCloak = captureCharacter();
            game.runGameRoutine([&] {
                auto &inventory = pParty->activeCharacter().inventory;
                if (InventoryEntry old = inventory.entry(ITEM_SLOT_CLOAK))
                    inventory.take(old);
                inventory.equip(ITEM_SLOT_CLOAK, Item(cloakId));
            });
            const RgbaImage cloakFrame = captureCharacter();
            ASSERT_EQ(cloakFrame.size(), beforeCloak.size());
            size_t changedPaperdollPixels = 0;
            for (int y = 0; y < 345; ++y)
                for (int x = 468; x < 640; ++x)
                    changedPaperdollPixels += beforeCloak[y][x] != cloakFrame[y][x];
            EXPECT_GT(changedPaperdollPixels, 250u);
            FileOutputStream cloakOutput{Path(cloakPath)};
            cloakOutput.write(png::encode(cloakFrame));
            cloakOutput.close();
        }

        RgbaImage buttonsOnly;
        game.runGameRoutine([&] {
            render->ClearTarget(Color(0, 0, 0));
            render->BeginScene2D();
            drawMM6CharacterScreenButtons(WINDOW_CHARACTER_INVENTORY);
            render->flushAndScale();
            buttonsOnly = render->MakeFullScreenshot();
        });
        ASSERT_EQ(buttonsOnly.size(), frame.size());
        for (GUIButton *button : {pCharacterScreen_StatsBtn, pCharacterScreen_SkillsBtn,
                                  pCharacterScreen_InventoryBtn, pCharacterScreen_AwardsBtn,
                                  pCharacterScreen_ExitBtn}) {
            ASSERT_NE(button, nullptr);
            size_t visiblePixels = 0;
            for (int y = button->rect.y; y < button->rect.y + button->rect.h; ++y)
                for (int x = button->rect.x; x < button->rect.x + button->rect.w; ++x) {
                    const Color pixel = buttonsOnly[y][x];
                    visiblePixels += pixel.r || pixel.g || pixel.b;
                }
            EXPECT_GT(visiblePixels, 100u);
        }

        game.runGameRoutine([] {
            static_cast<GUIWindow_CharacterRecord *>(pGUIWindow_CurrentMenu.get())->ShowStatsTab();
        });
        ASSERT_EQ(current_character_screen_window, WINDOW_CHARACTER_STATS);
        const RgbaImage statsFrame = captureCharacter();
        ASSERT_EQ(statsFrame.size(), inventoryFrame.size());
        size_t changedButtonPixels = 0;
        for (int y = 318; y < 351; ++y)
            for (int x = 22; x < 469; ++x)
                changedButtonPixels += inventoryFrame[y][x] != statsFrame[y][x];
        EXPECT_GT(changedButtonPixels, 100u);
        EXPECT_FALSE(window->isVisible());
        EXPECT_FALSE(window->grabsMouse());
    }
}
