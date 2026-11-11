#include <algorithm>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"

#include "GUI/GUIWindow.h"

GAME_TEST(MM7SideQuests, AngelStatuetteIsAcquiredFromTheMazeChest) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    auto chest = std::ranges::find_if(vChests, [](const Chest &candidate) {
        return !!candidate.inventory.find(ITEM_QUEST_ANGEL_STATUETTE);
    });
    ASSERT_NE(chest, vChests.end());
    const int chestId = chest - vChests.begin();
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(chestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int attempt = 0; attempt < 40 && !pParty->hasItem(ITEM_QUEST_ANGEL_STATUETTE) &&
                    pParty->pPickedItem.itemId != ITEM_QUEST_ANGEL_STATUETTE; ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_ANGEL_STATUETTE) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_ANGEL_STATUETTE);
}
