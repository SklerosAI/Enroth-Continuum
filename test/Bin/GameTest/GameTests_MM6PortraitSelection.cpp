#include <array>

#include "Testing/Game/GameTest.h"

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/Resources/ResourceManager.h"

#include "GUI/HudPortraitSelectionResources.h"

TEST(MM6PortraitSelectionRoute, OriginalFaceAndSpriteAnchors) {
    constexpr int faceX[] = {22, 135, 248, 360};
    constexpr int frameX[] = {16, 129, 242, 354};
    for (int index = 0; index < 4; ++index) {
        EXPECT_EQ(hudPortraitFacePosition(GameTitle::MM6, index, {34, 388}), Pointi(faceX[index], 383));
        EXPECT_EQ(hudPortraitSelectionPosition(GameTitle::MM6, index, {69, 91}, {25, 380}),
            Pointi(frameX[index], 377));
        EXPECT_EQ(hudPortraitFacePosition(GameTitle::MM7, index, {34, 388}), Pointi(34, 388));
        EXPECT_EQ(hudPortraitSelectionPosition(GameTitle::MM7, index, {69, 91}, {25, 380}),
            Pointi(25, 380));
    }
    EXPECT_EQ(hudPortraitSelectionPosition(GameTitle::MM6, -1, {69, 91}, {25, 380}), Pointi(25, 380));
    EXPECT_EQ(hudPortraitSelectionPosition(GameTitle::MM6, 4, {69, 91}, {25, 380}), Pointi(25, 380));
}

GAME_TEST(MM6PortraitSelection, OriginalPaletteAndFrameCycle) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const int id = pSpriteFrameTable->FastFindSprite("aframe1");
    ASSERT_GT(id, 0);
    const SpriteFrame &frame = pSpriteFrameTable->pSpriteSFrames[id];
    EXPECT_EQ(frame.textureName, "aframe1");
    EXPECT_EQ(frame.paletteId, 2);
    EXPECT_EQ(frame.frameLength, Duration::fromTicks(8));
    EXPECT_EQ(frame.animationLength, Duration::fromTicks(48));
    for (int index = 0; index < 6; ++index)
        EXPECT_EQ(pSpriteFrameTable->GetFrame(id, Duration::fromTicks(index * 8)), &frame + index);
    EXPECT_EQ(pSpriteFrameTable->GetFrame(id, Duration::fromTicks(48)), &frame);
    EXPECT_EQ(engine->resources()->spriteSize("aframe1"), Sizei(69, 91));

    std::array<GraphicsImage *, 6> selectionFrames = {};
    for (int index = 0; index < 6; ++index) {
        SCOPED_TRACE(index);
        const Duration time = Duration::fromTicks(index * 8);
        const SpriteFrame *selectedFrame = pSpriteFrameTable->GetFrame(id, time);
        GraphicsImage *selection = assets->getMM6SelectionFrame(time);
        ASSERT_NE(selection, nullptr);
        selectionFrames[index] = selection;
        EXPECT_EQ(assets->getMM6SelectionFrame(time + Duration::fromTicks(48)), selection);
        const RgbaImage &decoded = selection->rgba();
        RgbaImage indexed = engine->resources()->sprite(selectedFrame->textureName);
        const Palette palette = engine->resources()->palette(selectedFrame->paletteId);
        ASSERT_EQ(decoded.size(), indexed.size());
        int opaque = 0;
        int transparent = 0;
        for (size_t i = 0; i < indexed.pixels().size(); ++i) {
            const Color source = indexed.pixels()[i];
            const Color result = decoded.pixels()[i];
            if (source.a == 0) {
                EXPECT_EQ(result.a, 0);
                ++transparent;
            } else {
                EXPECT_EQ(result, palette.colors[source.r]);
                ++opaque;
            }
        }
        EXPECT_GT(opaque, 0);
        EXPECT_GT(transparent, 0);
    }
    EXPECT_EQ(assets->getMM6SelectionFrame(), selectionFrames.front());
    for (int index = 1; index < 6; ++index)
        EXPECT_NE(selectionFrames[index], selectionFrames[index - 1]);
}
