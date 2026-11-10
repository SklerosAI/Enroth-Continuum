#include <array>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

namespace {

struct Delivery {
    ItemId item;
    QuestBit active;
    QuestBit given;
};

constexpr std::array<Delivery, 6> deliveries = {{
    {ITEM_POTION_CURE_WOUNDS, QBIT_EMERALD_ISLAND_RED_POTION_ACTIVE, QBIT_EMERALD_ISLAND_RED_POTION_GIVEN},
    {ITEM_QUEST_SEASHELL, QBIT_EMERALD_ISLAND_SEASHELL_ACTIVE, QBIT_EMERALD_ISLAND_SEASHELL_GIVEN},
    {ITEM_LONGBOW, QBIT_EMERALD_ISLAND_LONGBOW_ACTIVE, QBIT_EMERALD_ISLAND_LONGBOW_GIVEN},
    {ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA, QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE, QBIT_EMERALD_ISLAND_FLOOR_TILE_GIVEN},
    {ITEM_QUEST_LUTE, QBIT_EMERALD_ISLAND_LUTE_ACTIVE, QBIT_EMERALD_ISLAND_LUTE_GIVEN},
    {ITEM_QUEST_WEALTHY_HAT, QBIT_EMERALD_ISLAND_HAT_ACTIVE, QBIT_EMERALD_ISLAND_HAT_GIVEN},
}};

}  // namespace

GAME_TEST(MM7ScavengerPath, JudgeAndShip) {
    // The Judge accepts held contest items one at a time. Markham offers the ship only after all six hand-ins.
    for (bool missingHat : {true, false}) {
        SCOPED_TRACE(fmt::format("missingHat={}", missingHat));
        test.prepareForNextTest();
        engine->config->debug.NoMargaret.setValue(true);
        engine->config->debug.NoDamage.setValue(true);
        game.startNewGame();
        auto winTape = tapes.questBit(QBIT_EMERALD_ISLAND_SCAVENGER_HUNT_WON);
        auto shipTape = tapes.questBit(QBIT_EMERALD_ISLAND_SHIP_OFFERED);
        auto escapedTape = tapes.questBit(QBIT_ESCAPED_EMERALD_ISLE);
        auto mapTape = tapes.map();
        test.startTaping();

        game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(16154, 8560, 128), 180); // Existing Issue201 door interaction.
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
        ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_EMERALD_ISLAND_MARKHAMS_HEADQUARTERS);
        game.pressGuiButton("House_Npc1"); // Thomas the Judge.
        game.tick();

        for (const Delivery &delivery : deliveries) {
            if (missingHat && delivery.item == ITEM_QUEST_WEALTHY_HAT)
                break;
            ASSERT_TRUE(pParty->_questBits[delivery.active]);
            ASSERT_FALSE(pParty->_questBits[delivery.given]);
            pParty->setHoldingItem(Item(delivery.item)); // Supply only the item; the Judge runs the hand-in event.
            game.pressGuiButton("HouseNpcDialogue_Option1");
            game.tick();
            ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
            ASSERT_FALSE(pParty->_questBits[delivery.active]);
            ASSERT_TRUE(pParty->_questBits[delivery.given]);
        }

        // The Judge checks whether all six tasks are complete on the next conversation after a hand-in.
        ASSERT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_SCAVENGER_HUNT_WON]);
        game.pressGuiButton("HouseNpcDialogue_Option1");
        game.tick();
        ASSERT_EQ(pParty->_questBits[QBIT_EMERALD_ISLAND_SCAVENGER_HUNT_WON], !missingHat);
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
        game.pressGuiButton("House_Npc0"); // Lord Markham.
        game.tick();
        if (missingHat) {
            EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_SHIP_OFFERED]);
            EXPECT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
            EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_HAT_ACTIVE]);
            EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_HAT_GIVEN]);
        } else {
            game.pressGuiButton("HouseNpcDialogue_Option0"); // Congratulations.
            game.tick();
            game.pressGuiButton("HouseNpcDialogue_Option0"); // Your ship.
            game.tick();
            ASSERT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_SHIP_OFFERED]);
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
            ASSERT_EQ(current_screen_type, SCREEN_GAME);
            game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(11008, 34, 193), 90); // Existing Issue201 ship interaction.
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            game.pressGuiButton("HouseNpcDialogue_Option0"); // Cast off.
            game.skipLoadingScreen();
            EXPECT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
            EXPECT_TRUE(pParty->_questBits[QBIT_ESCAPED_EMERALD_ISLE]);
        }

        EXPECT_EQ(winTape, missingHat ? tape(false) : tape(false, true));
        EXPECT_EQ(shipTape, missingHat ? tape(false) : tape(false, true));
        EXPECT_EQ(escapedTape, missingHat ? tape(false) : tape(false, true));
        EXPECT_EQ(mapTape.back(), missingHat ? MAP_EMERALD_ISLAND : MAP_HARMONDALE);
    }
}
