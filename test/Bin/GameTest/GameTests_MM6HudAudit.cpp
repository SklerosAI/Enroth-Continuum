#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Resources/ResourceManager.h"

#include "GUI/HudNpcArrowResources.h"

GAME_TEST(MM6HudAudit, NativeNpcArrows) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudNpcArrowResources arrows = hudNpcArrowResources(GameTitle::MM6,
        "IB-NPCLD-A", "IB-NPCRD-A");
    EXPECT_EQ(arrows.left, "npc_lup");
    EXPECT_EQ(arrows.right, "npc_rup");
    EXPECT_EQ(engine->resources()->iconSize(arrows.left), Sizei(14, 25));
    EXPECT_EQ(engine->resources()->iconSize(arrows.right), Sizei(14, 25));
}

TEST(MM6HudAuditResources, MM7KeepsAlignmentVariants) {
    const auto neutral = hudNpcArrowResources(GameTitle::MM7, "IB-NPCLD-A", "IB-NPCRD-A");
    const auto good = hudNpcArrowResources(GameTitle::MM7, "IB-NPCLD-B", "IB-NPCRD-B");
    const auto evil = hudNpcArrowResources(GameTitle::MM7, "IB-NPCLD-C", "IB-NPCRD-C");
    EXPECT_EQ(neutral.left, "IB-NPCLD-A");
    EXPECT_EQ(neutral.right, "IB-NPCRD-A");
    EXPECT_EQ(good.left, "IB-NPCLD-B");
    EXPECT_EQ(good.right, "IB-NPCRD-B");
    EXPECT_EQ(evil.left, "IB-NPCLD-C");
    EXPECT_EQ(evil.right, "IB-NPCRD-C");
}
