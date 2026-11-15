#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int charlesNpcId = 17;
constexpr int williamNpcId = 18;
constexpr int aliceNpcId = 54;
constexpr int crusaderOfferEventId = 51;
constexpr int crusaderReturnEventId = 52;
constexpr int heroOfferEventId = 53;
constexpr int heroReturnEventId = 54;
constexpr int aliceEventId = 55;
constexpr int villainOfferEventId = 56;
constexpr int villainReturnEventId = 57;
constexpr int deyjaSetagEventId = 503;
constexpr int aliceDoorEventId = 376;

struct PaladinPath {
    int promoterNpcId;
    int offerEventId;
    int returnEventId;
    QuestBit pathBit;
    QuestBit activeBit;
    Class promotedClass;
    AwardId qualifyingAward;
    AwardId honoraryAward;
    int goldReward;
    int reputationDelta;
    int greeting;
    HouseId aliceHouseAfter;
};

constexpr PaladinPath heroPath = {
    charlesNpcId, heroOfferEventId, heroReturnEventId, QBIT_LIGHT_PATH,
    QBIT_ALICE_HARGREAVES_RESCUE_ACTIVE, CLASS_HERO, AWARD_PROMOTION_HERO,
    AWARD_PROMOTION_HERO_HONORARY, 0, -10, 46, HOUSE_ERATHIA_QUIXOTE_RESIDENCE,
};

constexpr PaladinPath villainPath = {
    williamNpcId, villainOfferEventId, villainReturnEventId, QBIT_DARK_PATH,
    QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE, CLASS_VILLIAN, AWARD_PROMOTION_VILLIAN,
    AWARD_PROMOTION_VILLIAN_HONORARY, 10000, 10, 50, HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES,
};

bool eventChangesVariable(const EvtProgram &program, int eventId, EvtOpcode opcode, EvtVariable variable,
                          int value) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
                      instruction.data.variable_descr.value == value;
           });
}

bool eventSetsNpcTopic(const EvtProgram &program, int eventId, int npcId, int index, int topic) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == npcId &&
                      instruction.data.npc_topic_descr.index == index &&
                      instruction.data.npc_topic_descr.event_id == topic;
           });
}

bool eventMovesNpc(const EvtProgram &program, int eventId, int npcId, HouseId houseId) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_MoveNPC && instruction.data.npc_move_descr.npc_id == npcId &&
                      instruction.data.npc_move_descr.location_id == houseId;
           });
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    const std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                 npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    const char *prefix = current_screen_type == SCREEN_NPC_DIALOGUE ?
                         "NpcDialogue_Option" : "HouseNpcDialogue_Option";
    game.pressGuiButton(fmt::format("{}{}", prefix, option - topics.begin()));
    game.tick();
    return true;
}

bool npcEventVisible(NPCData *npc, unsigned int eventId) {
    const std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                 npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    return std::ranges::find(topics, line) != topics.end();
}

void leaveDialogue(EngineController &game) {
    for (int attempt = 0; attempt < 5 && current_screen_type != SCREEN_GAME; ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openOutdoorHouse(EngineController &game, MapId map, HouseId houseId, NPCData *npc) {
    game.teleportTo(map, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, houseId))
            doorIds.push_back(faceId);
    }
    ASSERT_FALSE(doorIds.empty());

    for (int faceId : doorIds) {
        const BLVFace &face = pOutdoor->faces[faceId];
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face.boundingBox.center() + direction * 200 * face.facePlane.normal;
            bool onWater = false;
            int floorFaceId = -1;
            position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
            int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                             -direction * face.facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(map, position, yaw);
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
        if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
            break;
    }

    ASSERT_TRUE(current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE);
    ASSERT_EQ(npc->house, houseId);
    auto entry = std::ranges::find(houseNpcs, npc, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openCharlesDialogue(EngineController &game, NPCData *charles) {
    openOutdoorHouse(game, MAP_ERATHIA, HOUSE_ERATHIA_QUIXOTE_RESIDENCE, charles);
}

bool deyjaSetagInteractionFinished(bool enterTower) {
    return enterTower ? current_screen_type == SCREEN_INPUT_BLV ||
                            engine->_currentLoadedMapId == MAP_WILLIAM_SETAGS_TOWER :
                        current_screen_type == SCREEN_NPC_DIALOGUE;
}

void useDeyjaSetagInteraction(EngineController &game, bool enterTower) {
    game.teleportTo(MAP_DEYJA, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(deyjaSetagEventId));

    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces) && !deyjaSetagInteractionFinished(enterTower);
         ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.eventId != deyjaSetagEventId)
            continue;
        if (face.Clickable()) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * 200 * face.facePlane.normal;
                bool onWater = false;
                int floorFaceId = -1;
                position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                game.teleportTo(MAP_DEYJA, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (deyjaSetagInteractionFinished(enterTower))
                    break;
            }
        } else if (face.Pressure_Plate()) {
            Vec3f position = face.boundingBox.center();
            position.z = face.boundingBox.z1 + 50;
            game.teleportTo(MAP_DEYJA, position, 0);
            game.tick(5);
        }
    }

    auto decoration = std::ranges::find(pLevelDecorations, deyjaSetagEventId, &LevelDecoration::uEventID);
    if (!deyjaSetagInteractionFinished(enterTower) && decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(MAP_DEYJA, decoration->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (deyjaSetagInteractionFinished(enterTower))
                break;
        }
    }

    ASSERT_TRUE(deyjaSetagInteractionFinished(enterTower));
    if (!enterTower)
        return;
    if (current_screen_type == SCREEN_INPUT_BLV) {
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
    }
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WILLIAM_SETAGS_TOWER);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

bool carriesTowerKey(const Actor &actor) {
    return actor.carriedItemId == ITEM_WILLIAMS_TOWER_KEY ||
           std::ranges::any_of(actor.items, [](const Item &item) {
               return item.itemId == ITEM_WILLIAMS_TOWER_KEY;
           });
}

bool partyHasTowerKey() {
    return pParty->pPickedItem.itemId == ITEM_WILLIAMS_TOWER_KEY ||
           pParty->hasItem(ITEM_WILLIAMS_TOWER_KEY);
}

void killAndLootTowerKeyCarrier(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WILLIAM_SETAGS_TOWER);
    auto carrier = std::ranges::find_if(pActors, carriesTowerKey);
    ASSERT_NE(carrier, pActors.end());
    const int carrierId = carrier - pActors.begin();
    ASSERT_FALSE(carrier->IsNotAlive());

    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }

    game.teleportTo(MAP_WILLIAM_SETAGS_TOWER, carrier->pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(carrierId);
    for (int attack = 0; attack < 400 && !carrier->IsNotAlive(); ++attack) {
        if (attack && attack % 10 == 0) {
            game.teleportTo(MAP_WILLIAM_SETAGS_TOWER, carrier->pos + Vec3f(0, -250, 0), 90);
            game.pointMouseAtActor(carrierId);
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
    }
    ASSERT_TRUE(carrier->IsNotAlive()) << "remaining HP: " << carrier->hp;
    for (int frame = 0; frame < 120 && carrier->aiState != Dead; ++frame)
        game.tick();
    ASSERT_EQ(carrier->aiState, Dead);

    auto viewport = std::ranges::find(pPrimaryWindow->vButtons, "Game_Viewport", &GUIButton::id);
    ASSERT_NE(viewport, pPrimaryWindow->vButtons.end());
    bool corpseVisible = false;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -250, 0), 90), std::pair(Vec3f(-250, 0, 0), 0),
                               std::pair(Vec3f(0, 250, 0), 270), std::pair(Vec3f(250, 0, 0), 180)}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_WILLIAM_SETAGS_TOWER, carrier->pos + offset, yaw, pitch);
            for (int y = 10; y < 470 && !corpseVisible; y += 10) {
                for (int x = 10; x < 630; x += 10) {
                    Pointi point(x, y);
                    if (!(*viewport)->Contains(point))
                        continue;
                    if (engine->PickMouse(engine->config->gameplay.MouseInteractionDepth.value(), x, y,
                                          &vis_anything_filter, &vis_face_filter).pid != Pid::actor(carrierId))
                        continue;
                    game.moveMouse(point);
                    game.tick();
                    corpseVisible = engine->PickMouseForInteraction().pid == Pid::actor(carrierId);
                    if (corpseVisible)
                        break;
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
    ASSERT_TRUE(partyHasTowerKey());

    if (pParty->pPickedItem.itemId != ITEM_WILLIAMS_TOWER_KEY) {
        for (Character &character : pParty->pCharacters) {
            InventoryEntry entry = character.inventory.find(ITEM_WILLIAMS_TOWER_KEY);
            if (!entry)
                continue;
            pParty->setHoldingItem(character.inventory.take(entry));
            break;
        }
    }
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_WILLIAMS_TOWER_KEY);
}

void speakWithAliceInTower(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WILLIAM_SETAGS_TOWER);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_WILLIAMS_TOWER_KEY);
    bool opened = false;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces) && !opened; ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != aliceDoorEventId)
            continue;
        for (float distance : {80.0f, 150.0f, 250.0f}) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
                int sectorId = 0;
                int floorFaceId = -1;
                float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
                if (floorZ == -30000 || floorFaceId == -1)
                    continue;
                position.z = floorZ;
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                game.teleportTo(MAP_WILLIAM_SETAGS_TOWER, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                opened = current_screen_type == SCREEN_NPC_DIALOGUE;
                if (opened)
                    break;
            }
            if (opened)
                break;
        }
    }
    ASSERT_TRUE(opened);
    NPCData &alice = pNPCStats->pNPCData[aliceNpcId];
    ASSERT_TRUE(selectNpcEvent(game, &alice, aliceEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    ASSERT_TRUE(alice.Hired());
    leaveDialogue(game);
}

void captureAliceAtGryphonheart(EngineController &game) {
    game.teleportTo(MAP_CASTLE_GRYPHONHEART, Vec3f(-6752, -4750, 1387), 270);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_NE(window_SpeakInHouse, nullptr);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES);
    NPCData &alice = pNPCStats->pNPCData[aliceNpcId];
    ASSERT_TRUE(selectNpcEvent(game, &alice, aliceEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    ASSERT_TRUE(alice.Hired());
    leaveDialogue(game);
}

void prepareSecondPromotionParty() {
    const std::array classes = {CLASS_CRUSADER, CLASS_CRUSADER, CLASS_PALADIN, CLASS_KNIGHT};
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = classes[i];
        pParty->pCharacters[i]._achievedAwardsBits.set(i < 2 ? AWARD_PROMOTION_CRUSADER :
                                                       AWARD_PROMOTION_CRUSADER_HONORARY);
    }
}

void openPromoterDialogue(EngineController &game, const PaladinPath &path, NPCData *promoter) {
    if (path.promoterNpcId == charlesNpcId)
        openCharlesDialogue(game, promoter);
    else
        useDeyjaSetagInteraction(game, false);
}

} // namespace

GAME_TEST(MM7PaladinPathPromotions, SourceContracts) {
    game.startNewGame();
    const NPCData &charles = pNPCStats->pNPCData[charlesNpcId];
    const NPCData &william = pNPCStats->pNPCData[williamNpcId];
    EXPECT_EQ(charles.house, HOUSE_ERATHIA_QUIXOTE_RESIDENCE);
    EXPECT_EQ(charles.dialogue_1_evt_id, crusaderOfferEventId);
    EXPECT_EQ(william.dialogue_1_evt_id, villainOfferEventId);

    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, crusaderReturnEventId, charlesNpcId, 0,
                                  heroOfferEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, heroOfferEventId, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_LIGHT_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, heroOfferEventId, EVENT_Set,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_ALICE_HARGREAVES_RESCUE_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, heroOfferEventId, charlesNpcId, 0,
                                  heroReturnEventId));
    EXPECT_TRUE(eventMovesNpc(engine->_globalEventMap, heroOfferEventId, aliceNpcId,
                              HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES));

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, villainOfferEventId, EVENT_Compare, VAR_Award,
                                     std::to_underlying(AWARD_PROMOTION_CRUSADER)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, villainOfferEventId, EVENT_Compare, VAR_Award,
                                     std::to_underlying(AWARD_PROMOTION_CRUSADER_HONORARY)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, villainOfferEventId, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_DARK_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, villainOfferEventId, EVENT_Set,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, villainOfferEventId, williamNpcId, 0,
                                  villainReturnEventId));
    EXPECT_TRUE(eventMovesNpc(engine->_globalEventMap, villainOfferEventId, aliceNpcId,
                              HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES));

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, aliceEventId, EVENT_Set,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_ALICE_HARGREAVES_TAKEN)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, aliceEventId, EVENT_Set, VAR_NPCs2,
                                     aliceNpcId));

    game.teleportTo(MAP_DEYJA, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(deyjaSetagEventId));
    const std::span<const EvtInstruction> deyja = engine->_localEventMap.function(deyjaSetagEventId);
    EXPECT_TRUE(std::ranges::any_of(deyja, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == williamNpcId;
    }));
    EXPECT_TRUE(std::ranges::any_of(deyja, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_MoveToMap && instruction.str == "MDT10.blv";
    }));

    game.teleportTo(MAP_WILLIAM_SETAGS_TOWER, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(aliceDoorEventId));
    const std::span<const EvtInstruction> towerDoor = engine->_localEventMap.function(aliceDoorEventId);
    EXPECT_TRUE(std::ranges::any_of(towerDoor, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_WILLIAMS_TOWER_KEY);
    }));
    EXPECT_TRUE(std::ranges::any_of(towerDoor, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == aliceNpcId;
    }));
    EXPECT_TRUE(std::ranges::any_of(pActors, carriesTowerKey));

    game.teleportTo(MAP_CASTLE_GRYPHONHEART, Vec3f(-6752, -4750, 1387), 270);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(aliceDoorEventId));
    const std::span<const EvtInstruction> castleDoor = engine->_localEventMap.function(aliceDoorEventId);
    EXPECT_TRUE(std::ranges::any_of(castleDoor, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value ==
                      std::to_underlying(QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE);
    }));
    EXPECT_TRUE(std::ranges::any_of(castleDoor, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse &&
               instruction.data.house_id == HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES;
    }));
}

GAME_TEST(MM7PaladinPathPromotions, CrusaderFirstPromotionControl) {
    game.startNewGame();
    NPCData &charles = pNPCStats->pNPCData[charlesNpcId];
    pParty->pCharacters[0].classType = CLASS_PALADIN;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const int reputationBefore = pParty->GetPartyReputation();

    openCharlesDialogue(game, &charles);
    ASSERT_TRUE(selectNpcEvent(game, &charles, crusaderOfferEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_WROMTHRAX_ACTIVE]);
    EXPECT_EQ(charles.dialogue_1_evt_id, crusaderReturnEventId);
    EXPECT_TRUE(charles.Hired());

    ASSERT_TRUE(selectNpcEvent(game, &charles, crusaderReturnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_WROMTHRAX_ACTIVE]);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_PALADIN);

    pParty->_questBits.set(QBIT_WROMTHRAX_KILLED);
    ASSERT_TRUE(selectNpcEvent(game, &charles, crusaderReturnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_WROMTHRAX_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WROMTHRAX_KILLED]);
    EXPECT_FALSE(charles.Hired());
    EXPECT_EQ(charles.house, HOUSE_ERATHIA_QUIXOTE_RESIDENCE);
    EXPECT_EQ(charles.dialogue_1_evt_id, heroOfferEventId);
    EXPECT_EQ(charles.greetingIndex, 43);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -5);

    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_CRUSADER);
    EXPECT_TRUE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_PROMOTION_CRUSADER]);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 30000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_KNIGHT);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CRUSADER_HONORARY]);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 15000);
    }
    leaveDialogue(game);
}

GAME_TEST(MM7PaladinPathPromotions, HeroOfferRequiresCrusaderTopicAndLightPath) {
    game.startNewGame();
    NPCData &charles = pNPCStats->pNPCData[charlesNpcId];
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_CRUSADER);

    openCharlesDialogue(game, &charles);
    EXPECT_FALSE(npcEventVisible(&charles, heroOfferEventId));
    EXPECT_FALSE(selectNpcEvent(game, &charles, heroOfferEventId));
    leaveDialogue(game);

    charles.dialogue_1_evt_id = heroOfferEventId;
    openCharlesDialogue(game, &charles);
    ASSERT_TRUE(selectNpcEvent(game, &charles, heroOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ALICE_HARGREAVES_RESCUE_ACTIVE]);
    EXPECT_EQ(charles.dialogue_1_evt_id, heroOfferEventId);
    leaveDialogue(game);

    pParty->_questBits.set(QBIT_DARK_PATH);
    openCharlesDialogue(game, &charles);
    ASSERT_TRUE(selectNpcEvent(game, &charles, heroOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ALICE_HARGREAVES_RESCUE_ACTIVE]);
    EXPECT_EQ(charles.dialogue_1_evt_id, heroOfferEventId);
    leaveDialogue(game);

    pParty->_questBits.reset(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    openCharlesDialogue(game, &charles);
    ASSERT_TRUE(selectNpcEvent(game, &charles, heroOfferEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_RESCUE_ACTIVE]);
    EXPECT_EQ(charles.dialogue_1_evt_id, heroReturnEventId);
    leaveDialogue(game);
}

GAME_TEST(MM7PaladinPathPromotions, VillainOfferRequiresCrusaderAwardAndDarkPath) {
    game.startNewGame();
    NPCData &william = pNPCStats->pNPCData[williamNpcId];

    useDeyjaSetagInteraction(game, false);
    ASSERT_TRUE(selectNpcEvent(game, &william, villainOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE]);
    EXPECT_EQ(william.dialogue_1_evt_id, villainOfferEventId);
    leaveDialogue(game);

    pParty->_questBits.set(QBIT_DARK_PATH);
    useDeyjaSetagInteraction(game, false);
    ASSERT_TRUE(selectNpcEvent(game, &william, villainOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE]);
    EXPECT_EQ(william.dialogue_1_evt_id, villainOfferEventId);
    leaveDialogue(game);

    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_CRUSADER);
    useDeyjaSetagInteraction(game, false);
    ASSERT_TRUE(selectNpcEvent(game, &william, villainOfferEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_CAPTURE_ACTIVE]);
    EXPECT_EQ(william.dialogue_1_evt_id, villainReturnEventId);
    leaveDialogue(game);
}

void runSecondPaladinPromotion(EngineController &game, const PaladinPath &path) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareSecondPromotionParty();
    pParty->_questBits.set(path.pathBit);
    NPCData &promoter = pNPCStats->pNPCData[path.promoterNpcId];
    NPCData &alice = pNPCStats->pNPCData[aliceNpcId];
    if (path.promoterNpcId == charlesNpcId)
        promoter.dialogue_1_evt_id = heroOfferEventId; // Crusader return event 52 establishes this endpoint.

    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openPromoterDialogue(game, path, &promoter);
    ASSERT_TRUE(selectNpcEvent(game, &promoter, path.offerEventId));
    ASSERT_TRUE(pParty->_questBits[path.activeBit]);
    ASSERT_EQ(promoter.dialogue_1_evt_id, path.returnEventId);
    ASSERT_EQ(alice.house, HOUSE_CASTLE_GRYPHONHEART_ALICE_HARGREAVES);

    const int goldBefore = pParty->GetGold();
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_FALSE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    ASSERT_FALSE(alice.Hired());
    ASSERT_TRUE(selectNpcEvent(game, &promoter, path.returnEventId));
    EXPECT_TRUE(pParty->_questBits[path.activeBit]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    EXPECT_FALSE(alice.Hired());
    EXPECT_EQ(promoter.dialogue_1_evt_id, path.returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    if (path.promoterNpcId == charlesNpcId) {
        useDeyjaSetagInteraction(game, true); // Setup teleport: authored Deyja tower entrance to MDT10.
        killAndLootTowerKeyCarrier(game);
        speakWithAliceInTower(game);
    } else {
        captureAliceAtGryphonheart(game); // Setup teleport: authored Alice-room door anchor.
    }
    ASSERT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    ASSERT_TRUE(alice.Hired());

    openPromoterDialogue(game, path, &promoter);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    const std::array experienceBeforeReturn = {
        pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience,
    };
    ASSERT_TRUE(selectNpcEvent(game, &promoter, path.returnEventId));
    EXPECT_FALSE(pParty->_questBits[path.activeBit]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    EXPECT_FALSE(alice.Hired());
    EXPECT_EQ(alice.house, path.aliceHouseAfter);
    EXPECT_EQ(promoter.dialogue_1_evt_id, 0u);
    EXPECT_EQ(promoter.greetingIndex, path.greeting);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, path.goldReward);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, path.reputationDelta);

    for (int i = 0; i < 4; ++i) {
        const bool qualifying = classBefore[i] == CLASS_CRUSADER;
        EXPECT_EQ(pParty->pCharacters[i].classType, qualifying ? path.promotedClass : classBefore[i]);
        auto expectedAwards = awardsBefore[i];
        expectedAwards.set(qualifying ? path.qualifyingAward : path.honoraryAward);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBeforeReturn[i], qualifying ? 80000 : 40000);
    }

    leaveDialogue(game);
    const std::array classAfter = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                   pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const int goldAfter = pParty->GetGold();
    const int reputationAfter = pParty->GetPartyReputation();
    openPromoterDialogue(game, path, &promoter);
    EXPECT_FALSE(npcEventVisible(&promoter, path.returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &promoter, path.returnEventId));
    EXPECT_FALSE(pParty->_questBits[path.activeBit]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ALICE_HARGREAVES_TAKEN]);
    EXPECT_FALSE(alice.Hired());
    EXPECT_EQ(promoter.dialogue_1_evt_id, 0u);
    EXPECT_EQ(promoter.greetingIndex, path.greeting);
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationAfter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}

GAME_TEST(MM7PaladinPathPromotions, HeroRescuesAliceAndPromotes) {
    runSecondPaladinPromotion(game, heroPath);
}

GAME_TEST(MM7PaladinPathPromotions, VillainCapturesAliceAndPromotes) {
    runSecondPaladinPromotion(game, villainPath);
}
