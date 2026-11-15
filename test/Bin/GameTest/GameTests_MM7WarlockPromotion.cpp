#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int anthonyGreenNpcId = 50;
constexpr int torAnwynNpcId = 51;
constexpr int babyDragonNpcId = 57;
constexpr int offerEventId = 102;
constexpr int returnEventId = 103;
constexpr int dragonCavesEntranceEventId = 503;
constexpr int torHouseEventId = 54;
constexpr int torHouseFaceId = 220;
constexpr int dragonEggSpriteId = 1;
constexpr int warlockGreetingId = 83;

bool eventChangesQuestBit(const EvtProgram &program, int eventId, EvtOpcode opcode, QuestBit bit) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == opcode &&
                      instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                      instruction.data.variable_descr.value == std::to_underlying(bit);
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

bool eventSetsNpcGreeting(const EvtProgram &program, int eventId, int npcId, int greeting) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SetNPCGreeting && instruction.data.npc_descr.npc_id == npcId &&
                      instruction.data.npc_descr.greeting == greeting;
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

void openTorDialogue(EngineController &game, NPCData *tor) {
    game.teleportTo(MAP_MOUNT_NIGHON, Vec3f(), 0);
    ASSERT_LT(torHouseFaceId, std::ssize(pOutdoor->faces));
    ASSERT_EQ(pOutdoor->faces[torHouseFaceId].eventId, torHouseEventId);
    ASSERT_TRUE(eventOpensHouse(torHouseEventId, HOUSE_MOUNT_NIGHON_ANWYN_RESIDENCE));

    const BLVFace &face = pOutdoor->faces[torHouseFaceId];
    bool opened = false;
    for (float direction : {1.0f, -1.0f}) {
        Vec3f position = face.boundingBox.center() + direction * 200 * face.facePlane.normal;
        bool onWater = false;
        int floorFaceId = -1;
        position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
        int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                         -direction * face.facePlane.normal.x) *
                              180 / std::numbers::pi);
        game.teleportTo(MAP_MOUNT_NIGHON, position, yaw);
        try {
            game.pointMouseAtFace(torHouseFaceId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        opened = current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE;
        if (opened)
            break;
    }

    ASSERT_TRUE(opened);
    ASSERT_EQ(tor->house, HOUSE_MOUNT_NIGHON_ANWYN_RESIDENCE);
    auto entry = std::ranges::find(houseNpcs, tor, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void prepareWarlockParty(AwardId prerequisite, bool lightPath, bool darkPath) {
    pParty->pCharacters[0].classType = CLASS_GREAT_DRUID;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    if (prerequisite != AWARD_INVALID)
        pParty->pCharacters[0]._achievedAwardsBits.set(prerequisite);
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
}

void useLandOfGiantsTransition(EngineController &game, int eventId) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LAND_OF_THE_GIANTS);
    auto finishPendingMove = [&] {
        for (int frame = 0; frame < 5 && current_screen_type != SCREEN_INPUT_BLV &&
                            engine->_currentLoadedMapId == MAP_LAND_OF_THE_GIANTS; ++frame)
            game.tick();
    };

    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces) &&
                         current_screen_type != SCREEN_INPUT_BLV &&
                         engine->_currentLoadedMapId == MAP_LAND_OF_THE_GIANTS; ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.eventId != eventId)
            continue;
        if (face.Clickable()) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                     std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
                game.teleportTo(MAP_LAND_OF_THE_GIANTS, face.boundingBox.center() + offset, yaw, -30);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                finishPendingMove();
                if (current_screen_type == SCREEN_INPUT_BLV ||
                    engine->_currentLoadedMapId != MAP_LAND_OF_THE_GIANTS)
                    break;
            }
        } else if (face.Pressure_Plate()) {
            Vec3f position = face.boundingBox.center();
            position.z = face.boundingBox.z1 + 50;
            game.teleportTo(MAP_LAND_OF_THE_GIANTS, position, 0);
            game.tick(5);
            finishPendingMove();
        }
    }

    auto decoration = std::ranges::find(pLevelDecorations, eventId, &LevelDecoration::uEventID);
    if (current_screen_type != SCREEN_INPUT_BLV && engine->_currentLoadedMapId == MAP_LAND_OF_THE_GIANTS &&
        decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(MAP_LAND_OF_THE_GIANTS, decoration->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            finishPendingMove();
            if (current_screen_type == SCREEN_INPUT_BLV ||
                engine->_currentLoadedMapId != MAP_LAND_OF_THE_GIANTS)
                break;
        }
    }

    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_DRAGON_CAVES);
}

void fillPartyInventories() {
    for (Character &character : pParty->pCharacters) {
        character.inventory.clear();
        const Sizei grid = character.inventory.gridSize();
        for (int x = 0; x < grid.w; ++x)
            for (int y = 0; y < grid.h; ++y)
                character.inventory.add({x, y}, Item(ITEM_BRASS_RING));
    }
}

void pickUpDragonEgg(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_DRAGON_CAVES);
    ASSERT_LT(dragonEggSpriteId, std::ssize(pSpriteObjects));
    ASSERT_EQ(pSpriteObjects[dragonEggSpriteId].containing_item.itemId, ITEM_QUEST_DRAGON_EGG);
    const Vec3f eggPosition = pSpriteObjects[dragonEggSpriteId].vPosition;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -200, 0), 90}, std::pair{Vec3f(-200, 0, 0), 0},
             std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(200, 0, 0), 180}}) {
        game.teleportTo(MAP_DRAGON_CAVES, eggPosition + offset, yaw, -10);
        try {
            game.pointMouseAtSprite(dragonEggSpriteId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        if (pParty->pPickedItem.itemId == ITEM_QUEST_DRAGON_EGG)
            break;
    }
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_DRAGON_EGG);
    EXPECT_EQ(pSpriteObjects[dragonEggSpriteId].uObjectDescID, 0);
}

} // namespace

GAME_TEST(MM7WarlockPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &tor = pNPCStats->pNPCData[torAnwynNpcId];
    EXPECT_EQ(tor.house, HOUSE_MOUNT_NIGHON_ANWYN_RESIDENCE);
    EXPECT_EQ(tor.dialogue_1_evt_id, offerEventId);
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, offerEventId, EVENT_Set,
                                     QBIT_DRAGON_EGG_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, torAnwynNpcId, 0, returnEventId));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, returnEventId, EVENT_Subtract,
                                     QBIT_DRAGON_EGG_ACTIVE));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, returnEventId, EVENT_Subtract,
                                     QBIT_ORACLE_RECOVERS_DRAGON_EGG));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, returnEventId, anthonyGreenNpcId,
                                    warlockGreetingId));

    game.teleportTo(MAP_MOUNT_NIGHON, Vec3f(), 0);
    ASSERT_LT(torHouseFaceId, std::ssize(pOutdoor->faces));
    EXPECT_EQ(pOutdoor->faces[torHouseFaceId].eventId, torHouseEventId);
    EXPECT_TRUE(eventOpensHouse(torHouseEventId, HOUSE_MOUNT_NIGHON_ANWYN_RESIDENCE));

    game.teleportTo(MAP_LAND_OF_THE_GIANTS, Vec3f(), 0);
    for (int eventId : {503, 504}) {
        ASSERT_TRUE(engine->_localEventMap.hasEvent(eventId));
        EXPECT_TRUE(std::ranges::any_of(engine->_localEventMap.function(eventId), [](const EvtInstruction &instruction) {
            return instruction.opcode == EVENT_MoveToMap;
        }));
    }
}

GAME_TEST(MM7WarlockPromotion, OfferRequiresGreatDruidAwardAndDarkPath) {
    const std::array cases = {
        std::tuple{AWARD_INVALID, false, true},
        std::tuple{AWARD_PROMOTION_GREAT_DRUID, false, false},
        std::tuple{AWARD_PROMOTION_GREAT_DRUID_HONORARY, true, false},
    };
    for (const auto &[prerequisite, lightPath, darkPath] : cases) {
        SCOPED_TRACE(fmt::format("award={}, light={}, dark={}", std::to_underlying(prerequisite),
                                 lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        prepareWarlockParty(prerequisite, lightPath, darkPath);
        NPCData &tor = pNPCStats->pNPCData[torAnwynNpcId];
        const int goldBefore = pParty->GetGold();
        openTorDialogue(game, &tor);
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &tor, offerEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_DRAGON_EGG_ACTIVE]);
        EXPECT_EQ(tor.dialogue_1_evt_id, offerEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }
}

GAME_TEST(MM7WarlockPromotion, DragonEggReturnPromotesGreatDruidAndGreetsTor) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareWarlockParty(AWARD_PROMOTION_GREAT_DRUID, false, true);
    NPCData &tor = pNPCStats->pNPCData[torAnwynNpcId];
    NPCData &anthony = pNPCStats->pNPCData[anthonyGreenNpcId];
    NPCData &babyDragon = pNPCStats->pNPCData[babyDragonNpcId];
    const int anthonyGreetingBefore = anthony.greetingIndex;
    const int torGreetingBefore = tor.greetingIndex;
    ASSERT_NE(torGreetingBefore, warlockGreetingId);
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openTorDialogue(game, &tor);
    const int offerGold = pParty->GetGold();
    const int offerReputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &tor, offerEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_DRAGON_EGG_ACTIVE]);
    EXPECT_EQ(tor.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    leaveDialogue(game);

    openTorDialogue(game, &tor);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_FALSE(pParty->hasItem(ITEM_QUEST_DRAGON_EGG));
    ASSERT_TRUE(selectNpcEvent(game, &tor, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_DRAGON_EGG_ACTIVE]);
    EXPECT_EQ(tor.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    EXPECT_EQ(tor.greetingIndex, torGreetingBefore);
    EXPECT_EQ(anthony.greetingIndex, anthonyGreetingBefore);
    leaveDialogue(game);

    game.teleportTo(MAP_LAND_OF_THE_GIANTS, Vec3f(), 0);
    if (current_screen_type == SCREEN_NPC_DIALOGUE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_BLASTER);
        pParty->pPickedItem.Reset(); // The unrelated first-arrival gift must not occupy the egg cursor.
    }
    fillPartyInventories();
    useLandOfGiantsTransition(game, dragonCavesEntranceEventId);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    pickUpDragonEgg(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    pParty->_questBits.set(QBIT_ORACLE_RECOVERS_DRAGON_EGG);

    openTorDialogue(game, &tor);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_DRAGON_EGG);
    ASSERT_TRUE(selectNpcEvent(game, &tor, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_DRAGON_EGG_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_DRAGON_EGG]);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_DRAGON_EGG));
    EXPECT_EQ(tor.dialogue_1_evt_id, 0u);
    EXPECT_TRUE(babyDragon.flags & NPC_HIRED);
    EXPECT_EQ(pParty->GetGold(), goldBeforeReturn);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);
    EXPECT_EQ(tor.greetingIndex, warlockGreetingId);
    EXPECT_EQ(anthony.greetingIndex, anthonyGreetingBefore);

    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_WARLOCK);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_WARLOCK);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 80000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_WARLOCK_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 40000);
    }

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    openTorDialogue(game, &tor);
    EXPECT_FALSE(npcEventVisible(&tor, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &tor, returnEventId));
    EXPECT_EQ(pParty->GetGold(), goldBeforeReturn);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);
    EXPECT_EQ(tor.greetingIndex, warlockGreetingId);
    EXPECT_EQ(anthony.greetingIndex, anthonyGreetingBefore);
    EXPECT_TRUE(babyDragon.flags & NPC_HIRED);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}
