#include <algorithm>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"

#include "GUI/GUIWindow.h"

GAME_TEST(MM7SideQuests, SeasonsStoleIsAcquiredFromHallOfThePitChest) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_SEASONS_STOLE_ACTIVE);

    game.teleportTo(MAP_HALL_OF_THE_PIT, Vec3f(), 0);
    auto chest = std::ranges::find_if(vChests, [](const Chest &candidate) {
        return !!candidate.inventory.find(ITEM_QUEST_SEASONS_STOLE);
    });
    ASSERT_NE(chest, vChests.end());
    const int chestId = chest - vChests.begin();
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(chestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int i = 0; i < 40 && !pParty->hasItem(ITEM_QUEST_SEASONS_STOLE) &&
                    pParty->pPickedItem.itemId != ITEM_QUEST_SEASONS_STOLE; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_SEASONS_STOLE) || pParty->pPickedItem.itemId == ITEM_QUEST_SEASONS_STOLE);
}
