#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

#include "MM7CampaignChain.h"

static bool carriesCube(const Actor &actor) {
    return actor.carriedItemId == ITEM_QUEST_CONTROL_CUBE || actor.items[0].itemId == ITEM_QUEST_CONTROL_CUBE ||
           actor.items[1].itemId == ITEM_QUEST_CONTROL_CUBE;
}

static bool partyHasCube() {
    return pParty->hasItem(ITEM_QUEST_CONTROL_CUBE) || pParty->pPickedItem.itemId == ITEM_QUEST_CONTROL_CUBE;
}

static DoorState templeDoorState(int doorId) {
    auto door = std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId);
    return door == pIndoor->doors.end() ? DOOR_CLOSED : door->state;
}

static void fightAndLootCubeCarrier(EngineController &game) {
    std::vector<int> carriers;
    for (const Actor &actor : pActors)
        if (actor.monsterId == MONSTER_BLASTERGUY_A && carriesCube(actor))
            carriers.push_back(actor.id);
    ASSERT_EQ(carriers.size(), 1u);
    const int carrierId = carriers.front();
    ASSERT_FALSE(pActors[carrierId].IsNotAlive());

    game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(carrierId);
    for (int attack = 0; attack < 400 && !pActors[carrierId].IsNotAlive(); ++attack) {
        if (attack && attack % 10 == 0) // The carrier walks around, the party follows it.
            game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + Vec3f(0, -250, 0), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
    }
    ASSERT_TRUE(pActors[carrierId].IsNotAlive());
    for (int frame = 0; frame < 120 && pActors[carrierId].aiState != Dead; ++frame) // The carrier falls.
        game.tick();
    ASSERT_EQ(pActors[carrierId].aiState, Dead);

    // pointMouseAtActor aims at the standing center, which misses the collapsed corpse.
    auto viewport = std::ranges::find(pPrimaryWindow->vButtons, "Game_Viewport", &GUIButton::id);
    ASSERT_NE(viewport, pPrimaryWindow->vButtons.end());
    bool corpseVisible = false;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -250, 0), 90), std::pair(Vec3f(-250, 0, 0), 0),
                               std::pair(Vec3f(0, 250, 0), 270), std::pair(Vec3f(250, 0, 0), 180)}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + offset, yaw, pitch);
            for (int y = 10; y < 470 && !corpseVisible; y += 10) {
                for (int x = 10; x < 630 && !corpseVisible; x += 10) {
                    Pointi point(x, y);
                    if (!(*viewport)->Contains(point))
                        continue;
                    if (engine->PickMouse(engine->config->gameplay.MouseInteractionDepth.value(), x, y,
                                          &vis_anything_filter, &vis_face_filter).pid != Pid::actor(carrierId))
                        continue;
                    game.moveMouse(point);
                    game.tick();
                    corpseVisible = engine->PickMouseForInteraction().pid == Pid::actor(carrierId);
                }
            }
            if (corpseVisible)
                break;
        }
        if (corpseVisible)
            break;
    }
    ASSERT_TRUE(corpseVisible);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

GAME_TEST(MM7CampaignChain, DISABLED_Chapter5) {
    // Fixtures: party strength, raised from C0's to level 200, speed and endurance 200, body resistance 1000 and a
    // melee damage bonus of 100 at full health, and kept by the later saves. Teleports to the Celeste town portal
    // arrival point, to the arrival points of the Temple of the Light, the Temple of the Dark, the Wine Cellar, Castle
    // Gloaming and the Pit, and to the chests, the altar lid button, the Pit door and the cube carrier inside them.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save005.mm7"));
    loadCampaignChainSlot(game, 4);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_ACTIVE]);
    for (Character &character : pParty->pCharacters) { // Tolberti heals faster than the C0 party hits, and his blasts eradicate on a failed body resistance roll.
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character.sResBodyBase = 1000;
        character._melee_dmg_bonus = 100;
        character.health = character.GetMaxHealth();
    }

    auto altarActiveTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_ACTIVE);
    auto altarFinishedTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    auto lightPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT);
    auto darkPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_DARK);
    auto cellarActiveTape = tapes.questBit(QBIT_WINE_CELLAR_ACTIVE);
    auto cellarFinishedTape = tapes.questBit(QBIT_WINE_CELLAR_FINISHED);
    auto vampireTape = tapes.questBit(QBIT_WINE_CELLAR_VAMPIRE_KILLED);
    auto jarsActiveTape = tapes.questBit(QBIT_SOUL_JARS_LIGHT_ACTIVE);
    auto jarsFinishedTape = tapes.questBit(QBIT_SOUL_JARS_LIGHT_FINISHED);
    auto caseTape = tapes.hasItem(ITEM_QUEST_CASE_OF_SOUL_JARS);
    auto archibaldTape = tapes.questBit(QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB);
    auto tolbertiActiveTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATION_ACTIVE);
    auto tolbertiTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATED);
    auto cubeTape = tapes.custom([] { return partyHasCube(); });
    auto resurectraHouseTape = tapes.custom([] { return pNPCStats->pNPCData[80].house; });
    auto resurectraTopicsTape = tapes.custom([] {
        return std::array{pNPCStats->pNPCData[80].dialogue_1_evt_id, pNPCStats->pNPCData[80].dialogue_2_evt_id,
                          pNPCStats->pNPCData[80].dialogue_4_evt_id};
    });
    auto mapTape = tapes.map();
    test.startTaping();

    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_1, 80);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 80, 147));
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_TEMPLE_OF_THE_LIGHT, Vec3f(-6784, -2832, 1649), 0); // Arrival point of the temple travel topic.
    lootCampaignChainChest(game, MAP_TEMPLE_OF_THE_LIGHT, 176, ITEM_QUEST_ALTAR_PIECE_LIGHT);
    game.teleportTo(MAP_TEMPLE_OF_THE_DARK, Vec3f(0, -4059, 513), 90); // Arrival point of the temple travel topic.
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_TEMPLE_OF_THE_DARK, 13, 1, [] { return templeDoorState(17) != DOOR_CLOSED; })); // The altar's lid covers the chest.
    for (int i = 0; i < 100 && templeDoorState(17) != DOOR_OPEN; i++)
        game.tick();
    lootCampaignChainChest(game, MAP_TEMPLE_OF_THE_DARK, 195, ITEM_QUEST_ALTAR_PIECE_DARK);
    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_1, 80);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 80, 148));
    leaveCampaignChainHouse(game);

    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_2, 81);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 81, 151));
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_WINE_CELLAR, Vec3f(601, -512, 1), 0); // Cellar entrance from Tatalia.
    useCampaignChainTransition(game, MAP_WINE_CELLAR, 501);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TATALIA);
    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_2, 81);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 81, 152));
    leaveCampaignChainHouse(game);

    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_3, 82);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 82, 153));
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(96, 3424, 1), 90); // Castle entrance from the Pit.
    lootCampaignChainChest(game, MAP_CASTLE_GLOAMING, 176, ITEM_QUEST_CASE_OF_SOUL_JARS);
    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_3, 82);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 82, 154));
    leaveCampaignChainHouse(game);

    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_4, 83);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 83, 155));
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_PIT, Vec3f(), 0);
    useCampaignChainTransition(game, MAP_PIT, 376);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SMALL_HOUSE);
    fightAndLootCubeCarrier(game);
    ASSERT_TRUE(partyHasCube());
    openCampaignChainCelesteHouse(game, HOUSE_CELESTE_HOSTEL_4, 83);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 83, 156));
    leaveCampaignChainHouse(game);

    EXPECT_EQ(altarActiveTape, tape(false, true, false));
    EXPECT_EQ(altarFinishedTape, tape(false, true));
    EXPECT_EQ(lightPieceTape, tape(false, true, false));
    EXPECT_EQ(darkPieceTape, tape(false, true, false));
    EXPECT_EQ(cellarActiveTape, tape(false, true, false));
    EXPECT_EQ(cellarFinishedTape, tape(false, true));
    EXPECT_EQ(vampireTape, tape(false, true));
    EXPECT_EQ(jarsActiveTape, tape(false, true, false));
    EXPECT_EQ(jarsFinishedTape, tape(false, true));
    EXPECT_EQ(caseTape, tape(false, true, false));
    EXPECT_EQ(archibaldTape, tape(false, true));
    EXPECT_EQ(tolbertiActiveTape, tape(false, true, false));
    EXPECT_EQ(tolbertiTape, tape(false, true));
    EXPECT_EQ(cubeTape, tape(false, true, false));
    EXPECT_EQ(resurectraHouseTape, tape(HOUSE_CELESTE_HOSTEL_1, HOUSE_CASTLE_LAMBENT_THRONE_ROOM));
    EXPECT_EQ(resurectraTopicsTape, tape(std::array{147u, 149u, 0u}, std::array{148u, 149u, 0u},
                                         std::array{0u, 149u, 0u}, std::array{0u, 149u, 223u}));
    EXPECT_EQ(mapTape, tape(MAP_CASTLE_LAMBENT, MAP_CELESTE, MAP_TEMPLE_OF_THE_LIGHT, MAP_TEMPLE_OF_THE_DARK, MAP_CELESTE,
                            MAP_WINE_CELLAR, MAP_TATALIA, MAP_CELESTE, MAP_CASTLE_GLOAMING, MAP_CELESTE, MAP_PIT,
                            MAP_SMALL_HOUSE, MAP_CELESTE));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 5);
}
