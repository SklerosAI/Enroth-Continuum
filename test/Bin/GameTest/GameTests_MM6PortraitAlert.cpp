#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Resources/ResourceManager.h"

#include "GUI/HudPortraitAlertResources.h"

TEST(MM6PortraitAlertRouting, AlertPrecedenceAndPositions) {
    EXPECT_EQ(hudPortraitAlertForFlags(1, 2, 3, false, false), 1);
    EXPECT_EQ(hudPortraitAlertForFlags(1, 2, 3, false, true), 2);
    EXPECT_EQ(hudPortraitAlertForFlags(1, 2, 3, true, false), 3);
    EXPECT_EQ(hudPortraitAlertForFlags(1, 2, 3, true, true), 3);

    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM6, 0, {30, 384}), Pointi(78, 378));
    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM6, 1, {145, 384}), Pointi(191, 378));
    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM6, 2, {260, 384}), Pointi(304, 378));
    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM6, 3, {375, 384}), Pointi(416, 378));
    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM7, 0, {30, 384}), Pointi(30, 384));
    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM6, -1, {30, 384}), Pointi(30, 384));
    EXPECT_EQ(hudPortraitAlertPosition(GameTitle::MM6, 4, {30, 384}), Pointi(30, 384));

    const auto mm7 = hudPortraitAlertResources(GameTitle::MM7, "IB-InitG-a", "IB-InitY-a", "IB-InitR-a");
    EXPECT_EQ(mm7.green, "IB-InitG-a");
    EXPECT_EQ(mm7.yellow, "IB-InitY-a");
    EXPECT_EQ(mm7.red, "IB-InitR-a");
}

GAME_TEST(MM6PortraitAlert, OriginalTimingGemIcons) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const auto icons = hudPortraitAlertResources(GameTitle::MM6,
        "IB-InitG-a", "IB-InitY-a", "IB-InitR-a");
    EXPECT_EQ(icons.green, "BUTTGEM");
    EXPECT_EQ(icons.yellow, "BUTTyel");
    EXPECT_EQ(icons.red, "BUTTred");
    EXPECT_EQ(engine->resources()->iconSize(icons.green), Sizei(7, 10));
    EXPECT_EQ(engine->resources()->iconSize(icons.yellow), Sizei(7, 10));
    EXPECT_EQ(engine->resources()->iconSize(icons.red), Sizei(7, 10));
}
