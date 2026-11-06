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
#include "Engine/Tables/TileTable.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIGame.h"

#include "Library/Image/Png.h"

#include "Utility/Memory/Blob.h"
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

    FileOutputStream output{Path(capturePath)};
    output.write(png::encode(frame));
    output.close();
}
