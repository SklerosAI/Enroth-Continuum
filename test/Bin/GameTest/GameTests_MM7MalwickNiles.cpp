#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <ranges>
#include <span>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

namespace {

constexpr int malwickId = 6;
constexpr int nilesId = 99;
constexpr int malwickCounterIndex = 4;
constexpr int castleAngerCounterIndex = 9;
constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

struct TapestrySource {
    QuestBit path;
    MapId castle;
};

constexpr std::array tapestrySources = {
    TapestrySource{QBIT_LIGHT_PATH, MAP_CASTLE_LAMBENT},
    TapestrySource{QBIT_DARK_PATH, MAP_CASTLE_GLOAMING},
};

enum class MalwickDeadline {
    VISIT_NILES,
    RETURN_TAPESTRY,
};

bool eventWritesQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add ||
            instruction.opcode == EVENT_Subtract) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventSpeaksNpc(int eventId, int npcId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [npcId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == npcId;
           });
}

unsigned int findNpcEvent(const NPCData &npc, auto predicate) {
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, NPCData &npc, unsigned int eventId) {
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    const char *prefix = current_screen_type == SCREEN_NPC_DIALOGUE ? "NpcDialogue_Option" : "HouseNpcDialogue_Option";
    game.pressGuiButton(fmt::format("{}{}", prefix, option - topics.begin()));
    game.tick();
    return true;
}

void loadMapAtPartyStart(EngineController &game, MapId map) {
    game.runGameRoutine([map] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE;
        uGameState = GAME_STATE_CHANGE_LOCATION;
        onMapLeave();
    });
    game.tick();
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, map);
}

template<class Done>
bool clickEventFace(EngineController &game, MapId map, int eventId, Done done) {
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        const int faceId = static_cast<int>(&face - pIndoor->faces.data());
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face.boundingBox.center() + direction * 150 * face.facePlane.normal;
            position.z = face.boundingBox.z1;
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
            if (done())
                return true;
        }
        for (float distance : {150.0f, 250.0f, 350.0f}) {
            for (const auto &[direction, yaw] : std::array{
                     std::pair{Vec3f(1, 0, 0), 180}, std::pair{Vec3f(-1, 0, 0), 0},
                     std::pair{Vec3f(0, 1, 0), 270}, std::pair{Vec3f(0, -1, 0), 90}}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                int sector = pIndoor->GetSector(position);
                int floorFace = -1;
                float floor = sector ? BLV_GetFloorLevel(position, sector, &floorFace) : -30000;
                if (floorFace == -1 || floor > face.boundingBox.z2 || floor < face.boundingBox.z1 - 200)
                    continue;
                position.z = floor;
                for (int pitch : {-15, -30, -45}) {
                    game.teleportTo(map, position, yaw, pitch);
                    try {
                        game.pointMouseAtFace(faceId);
                    } catch (const Exception &) {
                        continue;
                    }
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                    if (done())
                        return true;
                }
            }
        }
    }
    return false;
}

void triggerRestrictedCastleArea() {
    auto face = std::ranges::find(pIndoor->faces, 452, &BLVFace::eventId);
    ASSERT_NE(face, pIndoor->faces.end());
    const int faceId = static_cast<int>(face - pIndoor->faces.begin());
    runEvent(EvtContext(EVT_SOURCE_MAP, face->eventId, Pid::face(faceId)));
}

bool actorGroupIsAggressive(int group, bool aggressive) {
    return std::ranges::any_of(pActors, [group, aggressive](Actor &actor) {
        return actor.group == group && !actor.IsNotAlive() && static_cast<bool>(actor.attributes & ACTOR_AGGRESSOR) == aggressive;
    });
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

void leaveDialogue(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openOutdoorNpc(EngineController &game, MapId map, int npcId) {
    loadMapAtPartyStart(game, map);
    auto actor = std::ranges::find(pActors, npcId, &Actor::npcId);
    if (actor != pActors.end()) {
        bool pointed = false;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(200, 0, 0), 180}, std::pair{Vec3f(-200, 0, 0), 0},
                 std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(0, -200, 0), 90}}) {
            game.teleportTo(map, actor->pos + offset, yaw);
            try {
                game.pointMouseAtActor(actor->id);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        ASSERT_TRUE(pointed);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
        return;
    }
    auto decoration = std::ranges::find_if(pLevelDecorations, [npcId](const LevelDecoration &candidate) {
        return candidate.uEventID != 0 && eventSpeaksNpc(candidate.uEventID, npcId);
    });
    if (decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        bool pointed = false;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(map, decoration->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        ASSERT_TRUE(pointed);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
    } else {
        auto face = std::ranges::find_if(pOutdoor->faces, [npcId](const BLVFace &candidate) {
            return candidate.Clickable() && eventSpeaksNpc(candidate.eventId, npcId);
        });
        ASSERT_NE(face, pOutdoor->faces.end());
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face->boundingBox.center() + direction * 150 * face->facePlane.normal;
            position.z = face->boundingBox.z1;
            int yaw = std::lround(std::atan2(-direction * face->facePlane.normal.y,
                                            -direction * face->facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(map, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
}

void returnToHarmondale(EngineController &game) {
    loadMapAtPartyStart(game, MAP_TATALIA);
    loadMapAtPartyStart(game, MAP_HARMONDALE);
}

void triggerNilesLetter(EngineController &game, QuestBit path = QBIT_LIGHT_PATH) {
    pParty->_questBits.set(QBIT_MALWICK_FIREBALL_WAND_ACCEPTED);
    pParty->_questBits.set(path);
    pParty->PartyTimes.CounterEventValues[malwickCounterIndex] = pParty->GetPlayingTime();
    pParty->GetPlayingTime() += Duration::fromHours(1008);
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY) ||
                pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY);
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY)
        pParty->placeHeldItemInInventoryOrDrop();
    if (current_screen_type == SCREEN_NPC_DIALOGUE)
        leaveDialogue(game);
}

void acceptNilesTapestryJob(EngineController &game, QuestBit path = QBIT_LIGHT_PATH) {
    loadMapAtPartyStart(game, MAP_TATALIA);
    game.teleportTo(MAP_TATALIA, Vec3f(17920, 16652, 3120), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    NPCData &niles = pNPCStats->pNPCData[nilesId];
    const unsigned int tapestryOffer = findNpcEvent(niles, [path](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OnCanShowDialogItemCmp &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(path);
    });
    ASSERT_TRUE(std::ranges::any_of(engine->_globalEventMap.function(tapestryOffer), [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE) &&
               instruction.opcode != EVENT_Subtract;
    }));
    ASSERT_TRUE(selectNpcEvent(game, niles, tapestryOffer));
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());
    leaveDialogue(game);
}

void runSuccessfulTapestryRoute(EngineController &game, const TapestrySource &source) {
    game.startNewGame();
    triggerNilesLetter(game, source.path);
    acceptNilesTapestryJob(game, source.path);

    loadMapAtPartyStart(game, source.castle);
    ASSERT_EQ(engine->_persistentVariables.mapVars[4], 0);
    ASSERT_TRUE(clickEventFace(game, source.castle, 376, [] {
        return pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_TAKEN] &&
               (pParty->hasItem(ITEM_BIG_TAPESTRY) || pParty->pPickedItem.itemId == ITEM_BIG_TAPESTRY);
    }));
    if (pParty->pPickedItem.itemId == ITEM_BIG_TAPESTRY)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_BIG_TAPESTRY));

    openOutdoorNpc(game, MAP_TATALIA, nilesId);
    NPCData &niles = pNPCStats->pNPCData[nilesId];
    const unsigned int handIn = findNpcEvent(niles, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_BIG_TAPESTRY);
    });
    const int reputation = pParty->GetPartyReputation();
    const int gold = pParty->uNumGold;
    const int bankGold = pParty->uNumGoldInBank;
    const int experience = totalExperience();
    const auto awards = std::array{pParty->pCharacters[0]._achievedAwardsBits, pParty->pCharacters[1]._achievedAwardsBits,
                                   pParty->pCharacters[2]._achievedAwardsBits, pParty->pCharacters[3]._achievedAwardsBits};
    ASSERT_TRUE(selectNpcEvent(game, niles, handIn));

    EXPECT_FALSE(pParty->hasItem(ITEM_BIG_TAPESTRY));
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation + 5);
    EXPECT_EQ(pParty->uNumGold, gold);
    EXPECT_EQ(pParty->uNumGoldInBank, bankGold);
    EXPECT_EQ(totalExperience(), experience);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awards[i]);
    if (current_screen_type == SCREEN_NPC_DIALOGUE)
        leaveDialogue(game);
}

void runTapestryInvasionRoute(EngineController &game) {
    game.startNewGame();
    triggerNilesLetter(game);
    acceptNilesTapestryJob(game);
    const Time deadlineStartedAt = pParty->PartyTimes.CounterEventValues[malwickCounterIndex];
    pParty->uNumGoldInBank = 777;
    pParty->GetPlayingTime() = deadlineStartedAt + Duration::fromHours(672);
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    ASSERT_EQ(pParty->uNumGoldInBank, 0);
    ASSERT_TRUE(std::ranges::any_of(pActors, [](Actor &actor) {
        return actor.group == 9 && !actor.IsNotAlive();
    }));
    if (current_screen_type == SCREEN_NPC_DIALOGUE)
        leaveDialogue(game);
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void saveCheckpoint(EngineController &game, std::string_view label) {
    game.runGameRoutine([label] {
        saveGame(false, false, checkpointSlot, std::string(label));
    });
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

void reloadCheckpoint(EngineController &game, Blob &saved) {
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);
    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, checkpointFileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
}

}  // namespace

GAME_TEST(MM7MalwickNiles, RefusalKeepsMalwickProposalAvailable) {
    game.startNewGame();
    NPCData &malwick = pNPCStats->pNPCData[malwickId];

    openOutdoorNpc(game, MAP_EMERALD_ISLAND, malwickId);
    const unsigned int proposal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, proposal));
    const unsigned int refusal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 18;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, refusal));
    EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    EXPECT_FALSE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS));
    EXPECT_NE(pParty->pPickedItem.itemId, ITEM_ALACORN_WAND_OF_FIREBALLS);

    const unsigned int repeatedProposal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, repeatedProposal));
    const unsigned int acceptance = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MALWICK_FIREBALL_WAND_ACCEPTED) &&
               instruction.opcode != EVENT_Subtract;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, acceptance));
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS) ||
                pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS);
}

GAME_TEST(MM7MalwickNiles, SixWeekTriggerAndMissingTapestryRefusal) {
    game.startNewGame();
    NPCData &malwick = pNPCStats->pNPCData[malwickId];
    NPCData &niles = pNPCStats->pNPCData[nilesId];

    openOutdoorNpc(game, MAP_EMERALD_ISLAND, malwickId);
    const unsigned int proposal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, proposal));
    const unsigned int wandOffer = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MALWICK_FIREBALL_WAND_ACCEPTED) &&
               instruction.opcode != EVENT_Subtract;
    });
    const Time acceptedAt = pParty->GetPlayingTime();
    ASSERT_TRUE(selectNpcEvent(game, malwick, wandOffer));
    ASSERT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    ASSERT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS) ||
                pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS);
    if (pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS)
        pParty->placeHeldItemInInventoryOrDrop();
    leaveDialogue(game);

    // The production Light/Dark path choice starts this counter; that route is covered by MM7CampaignChain C3.
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->PartyTimes.CounterEventValues[malwickCounterIndex] = pParty->GetPlayingTime();
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->PartyTimes.CounterEventValues[malwickCounterIndex].isValid());
    const Time countdownStartedAt = pParty->PartyTimes.CounterEventValues[malwickCounterIndex];
    EXPECT_LE(countdownStartedAt, acceptedAt);

    pParty->GetPlayingTime() = countdownStartedAt + Duration::fromHours(1007);
    returnToHarmondale(game);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY));
    EXPECT_NE(pParty->pPickedItem.itemId, ITEM_MESSAGE_FROM_MR_STANTLEY);

    pParty->GetPlayingTime() += Duration::fromHours(1);
    returnToHarmondale(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY) ||
                pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY);
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY)
        pParty->placeHeldItemInInventoryOrDrop();
    if (current_screen_type == SCREEN_NPC_DIALOGUE)
        leaveDialogue(game);

    loadMapAtPartyStart(game, MAP_TATALIA);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(502));
    game.teleportTo(MAP_TATALIA, Vec3f(17920, 16652, 3120), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);

    const unsigned int tapestryOffer = findNpcEvent(niles, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE) &&
               instruction.opcode != EVENT_Subtract;
    });
    ASSERT_TRUE(selectNpcEvent(game, niles, tapestryOffer));
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());

    const int reputationBefore = pParty->GetPartyReputation();
    const unsigned int handIn = niles.dialogue_1_evt_id;
    ASSERT_TRUE(selectNpcEvent(game, niles, handIn));
    EXPECT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    EXPECT_FALSE(pParty->hasItem(ITEM_BIG_TAPESTRY));
}

GAME_TEST(MM7MalwickNiles, MissedDeadlinesStartRaidAndCloseHarmondaleServices) {
    for (MalwickDeadline deadline : {MalwickDeadline::VISIT_NILES, MalwickDeadline::RETURN_TAPESTRY}) {
        SCOPED_TRACE(fmt::format("deadline={}", std::to_underlying(deadline)));
        test.prepareForNextTest();
        game.startNewGame();
        triggerNilesLetter(game);

        int deadlineHours = 336;
        QuestBit activeBit = QBIT_MERCENARY_GUILD_VISIT_ACTIVE;
        if (deadline == MalwickDeadline::RETURN_TAPESTRY) {
            acceptNilesTapestryJob(game);
            deadlineHours = 672;
            activeBit = QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE;
        }
        const Time deadlineStartedAt = pParty->PartyTimes.CounterEventValues[malwickCounterIndex];
        pParty->uNumGoldInBank = 777;

        pParty->GetPlayingTime() = deadlineStartedAt + Duration::fromHours(deadlineHours - 1);
        loadMapAtPartyStart(game, MAP_HARMONDALE);
        ASSERT_TRUE(pParty->_questBits[activeBit]);
        ASSERT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
        ASSERT_EQ(pParty->uNumGoldInBank, 777);

        pParty->GetPlayingTime() += Duration::fromHours(1);
        loadMapAtPartyStart(game, MAP_HARMONDALE);
        ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
        EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
        EXPECT_EQ(pParty->uNumGoldInBank, 0);
        EXPECT_TRUE(std::ranges::any_of(pActors, [](Actor &actor) {
            return actor.group == 9 && !actor.IsNotAlive();
        }));
        if (current_screen_type == SCREEN_NPC_DIALOGUE)
            leaveDialogue(game);

        game.teleportTo(MAP_HARMONDALE, Vec3f(-6472, 14750, 32), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        EXPECT_EQ(current_screen_type, SCREEN_GAME);
    }
}

GAME_TEST(MM7MalwickNiles, PhysicalTapestriesCompleteBothPathReturns) {
    for (const TapestrySource &source : tapestrySources) {
        SCOPED_TRACE(fmt::format("path={} castle={}", std::to_underlying(source.path), std::to_underlying(source.castle)));
        test.prepareForNextTest();
        runSuccessfulTapestryRoute(game, source);
    }
}

GAME_TEST(MM7MalwickNiles, AlliedCastleHostilityExpiresAtThirtyDays) {
    for (const TapestrySource &source : tapestrySources) {
        SCOPED_TRACE(fmt::format("path={} castle={}", std::to_underlying(source.path), std::to_underlying(source.castle)));
        test.prepareForNextTest();
        game.startNewGame();
        pParty->_questBits.set(source.path);
        loadMapAtPartyStart(game, source.castle);
        triggerRestrictedCastleArea();
        const Time angerStartedAt = pParty->PartyTimes.CounterEventValues[castleAngerCounterIndex];
        ASSERT_TRUE(pParty->_questBits[QBIT_ALLIED_GUARDS_ANGERED]);

        pParty->GetPlayingTime() = angerStartedAt + Duration::fromHours(719);
        loadMapAtPartyStart(game, source.castle);
        EXPECT_TRUE(pParty->_questBits[QBIT_ALLIED_GUARDS_ANGERED]);
        EXPECT_EQ(engine->_persistentVariables.mapVars[4], 2);
        EXPECT_TRUE(actorGroupIsAggressive(4, true));
        EXPECT_TRUE(actorGroupIsAggressive(5, true));

        pParty->GetPlayingTime() += Duration::fromHours(1);
        loadMapAtPartyStart(game, source.castle);
        EXPECT_FALSE(pParty->_questBits[QBIT_ALLIED_GUARDS_ANGERED]);
        EXPECT_EQ(engine->_persistentVariables.mapVars[4], 0);
        EXPECT_TRUE(actorGroupIsAggressive(4, false));
        EXPECT_TRUE(actorGroupIsAggressive(5, false));
    }
}

GAME_TEST(MM7MalwickCheckpoint, DISABLED_WriteSuccess) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runSuccessfulTapestryRoute(game, tapestrySources[0]);
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TATALIA);
    saveCheckpoint(game, "Malwick success checkpoint");
}

GAME_TEST(MM7MalwickCheckpoint, DISABLED_ReloadSuccess) {
    assertDisposableHeadlessProfile();
    Blob saved;
    reloadCheckpoint(game, saved);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TATALIA);
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_TAKEN]);
    EXPECT_FALSE(pParty->hasItem(ITEM_BIG_TAPESTRY));
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}

GAME_TEST(MM7MalwickCheckpoint, DISABLED_WriteInvasion) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runTapestryInvasionRoute(game);
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    saveCheckpoint(game, "Malwick invasion checkpoint");
}

GAME_TEST(MM7MalwickCheckpoint, DISABLED_ReloadInvasion) {
    assertDisposableHeadlessProfile();
    Blob saved;
    reloadCheckpoint(game, saved);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    EXPECT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    EXPECT_EQ(pParty->uNumGoldInBank, 0);
    EXPECT_TRUE(std::ranges::any_of(pActors, [](Actor &actor) {
        return actor.group == 9 && !actor.IsNotAlive();
    }));
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
