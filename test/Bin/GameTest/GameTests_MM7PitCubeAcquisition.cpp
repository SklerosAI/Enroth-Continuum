#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

void openAdvisorHouse(EngineController &game) {
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, HOUSE_CELESTE_HOSTEL_4);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CELESTE_HOSTEL_4);
}

bool carriesCube(const Actor &actor) {
    if (actor.carriedItemId == ITEM_QUEST_CONTROL_CUBE)
        return true;
    return actor.items[0].itemId == ITEM_QUEST_CONTROL_CUBE || actor.items[1].itemId == ITEM_QUEST_CONTROL_CUBE;
}

}  // namespace

GAME_TEST(MM7PitCubeAcquisition, KillLootAndHandIn) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
    pParty->_questBits.set(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    pParty->_questBits.set(QBIT_WINE_CELLAR_FINISHED);
    pParty->_questBits.set(QBIT_SOUL_JARS_LIGHT_FINISHED);
    pNPCStats->pNPCData[83].house = HOUSE_CELESTE_HOSTEL_4;
    auto activeTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATION_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATED);
    auto cubeTape = tapes.hasItem(ITEM_QUEST_CONTROL_CUBE);
    test.startTaping();

    openAdvisorHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    auto advisor = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) {
        return npc.npc == &pNPCStats->pNPCData[83];
    });
    ASSERT_NE(advisor, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", advisor - houseNpcs.begin() + 1));
        game.tick();
    }
    ASSERT_EQ(pNPCStats->pNPCData[83].dialogue_1_evt_id, 155);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    ASSERT_FALSE(pParty->hasItem(ITEM_QUEST_CONTROL_CUBE));

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.teleportTo(MAP_PIT, Vec3f(), 0);
    auto pitDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 376 && face.Clickable();
    });
    ASSERT_NE(pitDoor, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*pitDoor);
    game.teleportTo(MAP_PIT, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SMALL_HOUSE);

    std::vector<int> carriers;
    for (const Actor &actor : pActors)
        if (actor.monsterId == MONSTER_BLASTERGUY_A && carriesCube(actor))
            carriers.push_back(actor.id);
    ASSERT_EQ(carriers.size(), 1u);
    ASSERT_GE(carriers.front(), 0);
    const int carrierId = carriers.front();
    ASSERT_FALSE(pActors[carrierId].IsNotAlive());
    const int initialHp = pActors[carrierId].hp;

    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }
    game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + Vec3f(0, -250, 0), 90);
    try {
        game.pointMouseAtActor(carrierId);
    } catch (const std::exception &error) {
        FAIL() << "initial target: " << error.what();
    }
    for (int attack = 0; attack < 400 && !pActors[carrierId].IsNotAlive(); ++attack) {
        SCOPED_TRACE(fmt::format("attack {}", attack));
        // Keep the party in melee range as the live actor moves; every hit still comes from the attack key.
        if (attack && attack % 10 == 0)
            game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + Vec3f(0, -250, 0), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
    }
    EXPECT_LT(pActors[carrierId].hp, initialHp);
    ASSERT_TRUE(pActors[carrierId].IsNotAlive()) << "remaining HP: " << pActors[carrierId].hp;
    for (int frame = 0; frame < 120 && pActors[carrierId].aiState != Dead; ++frame)
        game.tick();
    ASSERT_EQ(pActors[carrierId].aiState, Dead);

    SCOPED_TRACE("loot");
    bool corpseVisible = false;
    // The standing-center helper misses the collapsed corpse, so find a real interaction pixel in the viewport.
    auto viewport = std::ranges::find(pPrimaryWindow->vButtons, "Game_Viewport", &GUIButton::id);
    ASSERT_NE(viewport, pPrimaryWindow->vButtons.end());
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -250, 0), 90), std::pair(Vec3f(-250, 0, 0), 0),
                               std::pair(Vec3f(0, 250, 0), 270), std::pair(Vec3f(250, 0, 0), 180)}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + offset, yaw, pitch);
            for (int y = 10; y < 470 && !corpseVisible; y += 10) {
                for (int x = 10; x < 630; x += 10) {
                    Pointi point(x, y);
                    if (!(*viewport)->Contains(point))
                        continue;
                    if (engine->PickMouse(engine->config->gameplay.MouseInteractionDepth.value(), x, y,
                                          &vis_anything_filter, &vis_face_filter).pid == Pid::actor(carrierId)) {
                        game.moveMouse(point);
                        game.tick();
                        corpseVisible = engine->PickMouseForInteraction().pid == Pid::actor(carrierId);
                        if (corpseVisible)
                            break;
                    }
                }
            }
            if (corpseVisible)
                break;
        }
        if (corpseVisible)
            break;
    }
    ASSERT_TRUE(corpseVisible) << "actor state after targeting: " << static_cast<int>(pActors[carrierId].aiState);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    EXPECT_FALSE(carriesCube(pActors[carrierId]));
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_CONTROL_CUBE) || pParty->pPickedItem.itemId == ITEM_QUEST_CONTROL_CUBE);

    openAdvisorHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_CONTROL_CUBE));
    EXPECT_EQ(pNPCStats->pNPCData[80].house, HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(cubeTape, tape(false, true, false));
    test.stopTaping();
}
