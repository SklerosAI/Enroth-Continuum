#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM7CampaignChain.h"

static bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

static void openPitAdviser(EngineController &game, int npcId) {
    const HouseId houseId = pNPCStats->pNPCData[npcId].house;
    ASSERT_NE(houseId, HOUSE_INVALID);
    game.teleportTo(MAP_PIT, Vec3f(), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [houseId](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, houseId);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachCampaignChainFace(*door);
    game.teleportTo(MAP_PIT, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

static DoorState templeDoorState(int doorId) {
    auto door = std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId);
    return door == pIndoor->doors.end() ? DOOR_CLOSED : door->state;
}

static std::vector<int> chestEventsContaining(ItemId itemId) {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        for (const EvtInstruction &instruction : engine->_localEventMap.function(face.eventId)) {
            if (instruction.opcode != EVENT_OpenChest || instruction.data.chest_id < 0 ||
                instruction.data.chest_id >= vChests.size() || !vChests[instruction.data.chest_id].inventory.find(itemId))
                continue;
            if (std::ranges::find(result, face.eventId) == result.end())
                result.push_back(face.eventId);
        }
    }
    return result;
}

static std::vector<int> clankerDefenseEvents() {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        const auto &instructions = engine->_localEventMap.function(face.eventId);
        const bool disablesDefenses = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
            return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(QBIT_CLANKERS_LAB_DEFENSES_DESTROYED);
        });
        if (disablesDefenses && std::ranges::find(result, face.eventId) == result.end())
            result.push_back(face.eventId);
    }
    return result;
}

static bool carriesCube(const Actor &actor) {
    return actor.carriedItemId == ITEM_QUEST_CONTROL_CUBE || actor.items[0].itemId == ITEM_QUEST_CONTROL_CUBE ||
           actor.items[1].itemId == ITEM_QUEST_CONTROL_CUBE;
}

static bool partyHasCube() {
    return pParty->hasItem(ITEM_QUEST_CONTROL_CUBE) || pParty->pPickedItem.itemId == ITEM_QUEST_CONTROL_CUBE;
}

static void fightAndLootRobert(EngineController &game) {
    std::vector<int> carriers;
    for (const Actor &actor : pActors) {
        if (actor.monsterId == MONSTER_BLASTERGUY_B && carriesCube(actor))
            carriers.push_back(actor.id);
    }
    ASSERT_EQ(carriers.size(), 1u);
    const int carrierId = carriers.front();
    ASSERT_FALSE(pActors[carrierId].IsNotAlive());
    ASSERT_TRUE(pActors[carrierId].items[0].itemId == ITEM_BLASTER || pActors[carrierId].items[1].itemId == ITEM_BLASTER);

    game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(carrierId);
    for (int attack = 0; attack < 400 && !pActors[carrierId].IsNotAlive(); ++attack) {
        if (attack && attack % 10 == 0)
            game.teleportTo(MAP_SMALL_HOUSE, pActors[carrierId].pos + Vec3f(0, -250, 0), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
    }
    ASSERT_TRUE(pActors[carrierId].IsNotAlive());
    for (int frame = 0; frame < 120 && pActors[carrierId].aiState != Dead; ++frame)
        game.tick();
    ASSERT_EQ(pActors[carrierId].aiState, Dead);

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
    ASSERT_TRUE(partyHasCube());
}

GAME_TEST(MM7DarkCampaignChain, DISABLED_Chapter5) {
    // Fixtures: actors are disabled during item/event dungeon interactions, then restored for Robert's real fight.
    // The inherited C0 party is strengthened against Robert's blaster. Teleports approach adviser houses, dungeon
    // interactions and the live target; every item comes from its authored chest or actor and every hand-in uses the
    // production dialogue path.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save005.mm7"));
    loadCampaignChainSlot(game, 4);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_PIT);
    ASSERT_TRUE(pParty->_questBits[QBIT_DARK_GUILD_JOINED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_ACTIVE]);
    for (Character &character : pParty->pCharacters) {
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character.sResBodyBase = 1000;
        character._melee_dmg_bonus = 100;
        character.health = character.GetMaxHealth();
    }

    auto altarActiveTape = tapes.questBit(QBIT_ALTAR_PIECES_DARK_ACTIVE);
    auto altarFinishedTape = tapes.questBit(QBIT_ALTAR_PIECES_DARK_FINISHED);
    auto lightPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT);
    auto darkPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_DARK);
    auto jarsActiveTape = tapes.questBit(QBIT_SOUL_JARS_DARK_ACTIVE);
    auto jarsFinishedTape = tapes.questBit(QBIT_SOUL_JARS_DARK_FINISHED);
    auto caseTape = tapes.hasItem(ITEM_QUEST_CASE_OF_SOUL_JARS);
    auto clankerActiveTape = tapes.questBit(QBIT_CLANKERS_LAB_ACTIVE);
    auto defensesTape = tapes.questBit(QBIT_CLANKERS_LAB_DEFENSES_DESTROYED);
    auto clankerFinishedTape = tapes.questBit(QBIT_CLANKERS_LAB_FINISHED);
    auto archibaldMovedTape = tapes.questBit(QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB);
    auto robertActiveTape = tapes.questBit(QBIT_ROBERT_THE_WISE_ASSASSINATION_ACTIVE);
    auto robertFinishedTape = tapes.questBit(QBIT_ROBERT_THE_WISE_ASSASSINATED);
    auto cubeTape = tapes.custom([] { return partyHasCube(); });
    auto mapTape = tapes.map();
    test.startTaping();

    engine->config->debug.NoActors.setValue(true);
    openPitAdviser(game, 84); // Kastore.
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_ACTIVE]);
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_TEMPLE_OF_THE_LIGHT, Vec3f(-6784, -2832, 1649), 0);
    lootCampaignChainChest(game, MAP_TEMPLE_OF_THE_LIGHT, 176, ITEM_QUEST_ALTAR_PIECE_LIGHT);
    game.teleportTo(MAP_TEMPLE_OF_THE_DARK, Vec3f(0, -4059, 513), 90);
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_TEMPLE_OF_THE_DARK, 13, 1, [] {
        return templeDoorState(17) != DOOR_CLOSED;
    }));
    for (int i = 0; i < 100 && templeDoorState(17) != DOOR_OPEN; ++i)
        game.tick();
    ASSERT_EQ(templeDoorState(17), DOOR_OPEN);
    lootCampaignChainChest(game, MAP_TEMPLE_OF_THE_DARK, 195, ITEM_QUEST_ALTAR_PIECE_DARK);
    openPitAdviser(game, 84);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_DARK_FINISHED]);
    leaveCampaignChainHouse(game);

    openPitAdviser(game, 85); // Maximus.
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_DARK_ACTIVE]);
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_THUNDERFIST_MOUNTAIN, Vec3f(), 0);
    const std::vector<int> soulJarEvents = chestEventsContaining(ITEM_QUEST_CASE_OF_SOUL_JARS);
    ASSERT_EQ(soulJarEvents.size(), 1u);
    lootCampaignChainChest(game, MAP_THUNDERFIST_MOUNTAIN, soulJarEvents.front(), ITEM_QUEST_CASE_OF_SOUL_JARS);
    openPitAdviser(game, 85);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_DARK_FINISHED]);
    leaveCampaignChainHouse(game);

    openPitAdviser(game, 86); // Dark Shade.
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_CLANKERS_LAB_ACTIVE]);
    leaveCampaignChainHouse(game);
    game.teleportTo(MAP_CLANKERS_LABORATORY, Vec3f(), 0);
    const std::vector<int> defenseEvents = clankerDefenseEvents();
    ASSERT_EQ(defenseEvents.size(), 1u);
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_CLANKERS_LABORATORY, defenseEvents.front(), 1, [] {
        return pParty->_questBits[QBIT_CLANKERS_LAB_DEFENSES_DESTROYED];
    }));
    openPitAdviser(game, 86);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_CLANKERS_LAB_FINISHED]);
    leaveCampaignChainHouse(game);

    openPitAdviser(game, 87); // Tolberti.
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ROBERT_THE_WISE_ASSASSINATION_ACTIVE]);
    leaveCampaignChainHouse(game);
    engine->config->debug.NoActors.setValue(false);
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    useCampaignChainTransition(game, MAP_CELESTE, 376);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SMALL_HOUSE);
    fightAndLootRobert(game);
    openPitAdviser(game, 87);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ROBERT_THE_WISE_ASSASSINATED]);
    leaveCampaignChainHouse(game);

    EXPECT_EQ(altarActiveTape, tape(false, true, false));
    EXPECT_EQ(altarFinishedTape, tape(false, true));
    EXPECT_EQ(lightPieceTape, tape(false, true, false));
    EXPECT_EQ(darkPieceTape, tape(false, true, false));
    EXPECT_EQ(jarsActiveTape, tape(false, true, false));
    EXPECT_EQ(jarsFinishedTape, tape(false, true));
    EXPECT_EQ(caseTape, tape(false, true, false));
    EXPECT_EQ(clankerActiveTape, tape(false, true, false));
    EXPECT_EQ(defensesTape, tape(false, true));
    EXPECT_EQ(clankerFinishedTape, tape(false, true));
    EXPECT_EQ(archibaldMovedTape, tape(false, true));
    EXPECT_EQ(robertActiveTape, tape(false, true, false));
    EXPECT_EQ(robertFinishedTape, tape(false, true));
    EXPECT_EQ(cubeTape, tape(false, true, false));
    EXPECT_EQ(pNPCStats->pNPCData[84].house, HOUSE_CASTLE_GLOAMING_THRONE_ROOM_2);
    for (const Character &character : pParty->pCharacters)
        EXPECT_EQ(character.getSkillValue(SKILL_BLASTER), CombinedSkillValue::novice());
    EXPECT_EQ(mapTape, tape(MAP_PIT, MAP_TEMPLE_OF_THE_LIGHT, MAP_TEMPLE_OF_THE_DARK, MAP_PIT,
                            MAP_THUNDERFIST_MOUNTAIN, MAP_PIT, MAP_CLANKERS_LABORATORY, MAP_PIT,
                            MAP_CELESTE, MAP_SMALL_HOUSE, MAP_PIT));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 5);
}
