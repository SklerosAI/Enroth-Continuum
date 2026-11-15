#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
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

constexpr int catherineId = 69;
constexpr int parsonId = 70;
constexpr int lorenId = 71;
constexpr int falseLorenId = 72;
constexpr int humanAmbassadorId = 67;
constexpr int elvenAmbassadorId = 68;
constexpr int judgeGreyId = 74;
constexpr int goodJudgeId = 77;
constexpr int evilJudgeId = 78;
constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperience = {13500, 13600, 13700, 13800};

enum class WarJobOutcome {
    REAL,
    DECOY,
    EXPIRED,
};

enum class TrumpetRecipient {
    CATHERINE,
    PARSON,
    ARBITER,
};

enum class WarResultState {
    NONE,
    REAL,
    FAKE,
};

enum class WarWinner {
    HUMAN,
    ELF,
    NEUTRAL,
};

enum class ArbiterPath {
    LIGHT,
    DARK,
};

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    for (float direction : {1.0f, -1.0f}) {
        Vec3f position = face.boundingBox.center() + direction * 150 * face.facePlane.normal;
        int sectorId = 0;
        int floorFaceId = -1;
        float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
        if (floorZ == -30000 || floorFaceId == -1)
            continue;
        position.z = floorZ;
        int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                         -direction * face.facePlane.normal.x) *
                              180 / std::numbers::pi);
        return {position, yaw};
    }
    return {face.boundingBox.center(), 0};
}

bool eventChangesQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Add || instruction.opcode == EVENT_Set ||
            instruction.opcode == EVENT_Subtract) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventUsesVariable(const EvtInstruction &instruction, EvtOpcode opcode, EvtVariable variable,
                       int value, int targetStep = -1) {
    return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
           instruction.data.variable_descr.value == value &&
           (targetStep == -1 || instruction.target_step == targetStep);
}

bool eventUsesVariable(const EvtProgram &program, int eventId, int step, EvtOpcode opcode,
                       EvtVariable variable, int value, int targetStep = -1) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step &&
                      eventUsesVariable(instruction, opcode, variable, value, targetStep);
           });
}

int countGlobalVariableUses(EvtOpcode opcode, EvtVariable variable, int value) {
    int result = 0;
    for (int eventId = 1; eventId < 1000; ++eventId) {
        if (!engine->_globalEventMap.hasEvent(eventId))
            continue;
        result += std::ranges::count_if(engine->_globalEventMap.function(eventId),
                                        [=](const EvtInstruction &instruction) {
            return eventUsesVariable(instruction, opcode, variable, value);
        });
    }
    return result;
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventChangesQuestBit(int eventId, QuestBit bit) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [bit](const EvtInstruction &instruction) {
               return eventChangesQuestBit(instruction, bit);
           });
}

unsigned int findNpcEvent(int npcId, auto predicate) {
    const NPCData &npc = pNPCStats->pNPCData[npcId];
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, int npcId, unsigned int eventId) {
    NPCData &npc = pNPCStats->pNPCData[npcId];
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
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
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

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openThroneRoom(EngineController &game, MapId map, HouseId house, int npcId) {
    loadMapAtPartyStart(game, map);
    auto door = std::ranges::find_if(pIndoor->faces, [house](const BLVFace &face) {
        return face.Clickable() && eventOpensHouse(face.eventId, house);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(map, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), house);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

void openCatherine(EngineController &game) {
    openThroneRoom(game, MAP_CASTLE_GRYPHONHEART, HOUSE_CASTLE_GRYPHONHEART_THRONE_ROOM, catherineId);
}

void openParson(EngineController &game) {
    openThroneRoom(game, MAP_CASTLE_NAVAN, HOUSE_CASTLE_NAVAN_THRONE_ROOM, parsonId);
}

void acquireLoren(EngineController &game) {
    loadMapAtPartyStart(game, MAP_TULAREAN_CAVES);
    auto cell = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && eventChangesQuestBit(face.eventId, QBIT_LOREN_STEEL_FREED);
    });
    ASSERT_NE(cell, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*cell);
    game.teleportTo(MAP_TULAREAN_CAVES, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_FREED]);
    EXPECT_TRUE(pNPCStats->pNPCData[lorenId].Hired());
    if (current_screen_type == SCREEN_NPC_DIALOGUE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
}

void acquireRiverstridePlans(EngineController &game) {
    loadMapAtPartyStart(game, MAP_FORT_RIVERSTRIDE);
    auto vault = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && eventChangesQuestBit(face.eventId, QBIT_RIVERSTRIDE_VAULT_OPENED);
    });
    ASSERT_NE(vault, pIndoor->faces.end());
    auto [vaultPosition, vaultYaw] = approachFace(*vault);
    game.teleportTo(MAP_FORT_RIVERSTRIDE, vaultPosition, vaultYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(30);
    ASSERT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_VAULT_OPENED]);

    auto plans = std::ranges::find(pSpriteObjects, ITEM_MESSAGE_RIVERSTRIDE_PLANS,
                                   [](const SpriteObject &object) { return object.containing_item.itemId; });
    ASSERT_NE(plans, pSpriteObjects.end());
    const int spriteId = plans - pSpriteObjects.begin();
    bool taken = false;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -200, 0), 90), std::pair(Vec3f(-200, 0, 0), 0),
                               std::pair(Vec3f(0, 200, 0), 270), std::pair(Vec3f(200, 0, 0), 180)}) {
        Vec3f position = plans->vPosition + offset;
        int sectorId = 0;
        int floorFaceId = -1;
        float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
        if (floorZ == -30000 || floorFaceId == -1)
            continue;
        position.z = floorZ;
        game.teleportTo(MAP_FORT_RIVERSTRIDE, position, yaw, -10);
        try {
            game.pointMouseAtSprite(spriteId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        taken = pParty->pPickedItem.itemId == ITEM_MESSAGE_RIVERSTRIDE_PLANS ||
                pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS);
        if (taken)
            break;
    }
    ASSERT_TRUE(taken);
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_RIVERSTRIDE_PLANS)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS));
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

void waitForPartyDeathRecovery(EngineController &game, int previousDeaths) {
    for (int i = 0; i < 128 && pParty->uNumDeaths == previousDeaths; ++i)
        game.tick();
    ASSERT_EQ(pParty->uNumDeaths, previousDeaths + 1);
    for (int i = 0; i < 256 && uGameState != GAME_STATE_PLAYING; ++i)
        game.tick();
    ASSERT_EQ(uGameState, GAME_STATE_PLAYING);
    for (const Character &character : pParty->pCharacters)
        ASSERT_TRUE(character.CanAct());
    game.goToGame();
}

WarWinner expectedWarWinner(WarResultState plans, WarResultState loren,
                            TrumpetRecipient recipient) {
    if (plans == WarResultState::REAL) {
        if (loren == WarResultState::FAKE)
            return WarWinner::ELF;
        if (loren == WarResultState::REAL) {
            if (recipient == TrumpetRecipient::CATHERINE)
                return WarWinner::HUMAN;
            if (recipient == TrumpetRecipient::PARSON)
                return WarWinner::ELF;
            return WarWinner::NEUTRAL;
        }
        return recipient == TrumpetRecipient::PARSON ? WarWinner::ELF : WarWinner::HUMAN;
    }

    if (plans == WarResultState::FAKE && loren == WarResultState::FAKE) {
        if (recipient == TrumpetRecipient::CATHERINE)
            return WarWinner::HUMAN;
        if (recipient == TrumpetRecipient::PARSON)
            return WarWinner::ELF;
        return WarWinner::NEUTRAL;
    }

    if (plans == WarResultState::NONE && loren == WarResultState::FAKE &&
        recipient == TrumpetRecipient::PARSON)
        return WarWinner::ELF;
    return WarWinner::HUMAN;
}

void setWarResultBits(WarResultState plans, WarResultState loren,
                      TrumpetRecipient recipient) {
    if (plans == WarResultState::REAL)
        pParty->_questBits.set(QBIT_RIVERSTRIDE_PLANS_GIVEN);
    else if (plans == WarResultState::FAKE)
        pParty->_questBits.set(QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN);
    else
        pParty->_questBits.set(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED);

    if (loren == WarResultState::REAL)
        pParty->_questBits.set(QBIT_LOREN_STEEL_RESCUED);
    else if (loren == WarResultState::FAKE)
        pParty->_questBits.set(QBIT_FALSE_LOREN_GIVEN);
    else
        pParty->_questBits.set(QBIT_FALSE_LOREN_REVEALED);

    if (recipient == TrumpetRecipient::CATHERINE)
        pParty->_questBits.set(QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA);
    else if (recipient == TrumpetRecipient::PARSON)
        pParty->_questBits.set(QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE);
    else
        pParty->_questBits.set(QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER);
}

void prepareWarJobs() {
    pParty->_questBits.set(QBIT_HARMONDALE_REBUILT);
    pParty->SetGold(1000);
    pParty->uNumGoldInBank = 1000;
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].experience = 1000 + i * 100;
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void acquireGryphonheartTrumpet(EngineController &game) {
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_TIMER_STARTED]);

    // Exercise the authored timer start and map-reload event while keeping the test headless and fast.
    pParty->GetPlayingTime() += Duration::fromDays(57);
    loadMapAtPartyStart(game, MAP_TULAREAN_FOREST);
    ASSERT_TRUE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_MESSENGER_MET]);
    ASSERT_TRUE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_ACTIVE]);
    if (current_screen_type == SCREEN_NPC_DIALOGUE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    if (pParty->pPickedItem.itemId != ITEM_NULL)
        pParty->placeHeldItemInInventoryOrDrop();

    constexpr int trumpetChestId = 0;
    ASSERT_TRUE(engine->_localEventMap.hasEvent(170));
    ASSERT_TRUE(std::ranges::any_of(engine->_localEventMap.function(170), [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == trumpetChestId;
    }));
    ASSERT_LT(trumpetChestId, vChests.size());
    ASSERT_TRUE(vChests[trumpetChestId].inventory.find(ITEM_QUEST_GRYPHONHEARTS_TRUMPET));
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(trumpetChestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int attempt = 0; attempt < 20 && !pParty->hasItem(ITEM_QUEST_GRYPHONHEARTS_TRUMPET); ++attempt) {
        if (pParty->pPickedItem.itemId != ITEM_NULL)
            pParty->placeHeldItemInInventoryOrDrop();
        Chest::GrabItem(true);
        game.tick();
    }
    if (pParty->pPickedItem.itemId != ITEM_NULL)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_GRYPHONHEARTS_TRUMPET));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openJudgeGrey(EngineController &game) {
    game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_BARROW_DOWNS_HARMONDALE_ARBITER);
    auto judge = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[74];
    });
    ASSERT_NE(judge, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", judge - houseNpcs.begin()));
        game.tick();
    }
}

void installDarkArbiterAfterBalancedWar(EngineController &game) {
    pParty->_questBits.set(QBIT_RIVERSTRIDE_PLANS_GIVEN);
    pParty->_questBits.set(QBIT_LOREN_STEEL_RESCUED);
    acquireGryphonheartTrumpet(game);
    openJudgeGrey(game);
    const unsigned int handIn = findNpcEvent(74, [](const EvtInstruction &instruction) {
        return eventChangesQuestBit(instruction, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER) &&
               instruction.opcode != EVENT_Subtract;
    });
    ASSERT_TRUE(selectNpcEvent(game, 74, handIn));
    ASSERT_TRUE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER]);
    leaveHouse(game);

    // Counter 3 started when Harmondale loaded before the trumpet battle. Advance to the authored 112-day messenger.
    pParty->GetPlayingTime() += Duration::fromDays(56);
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_ARBITER_MESSENGER_MET]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ARBITER_CHOICE_ACTIVE]);
    if (current_screen_type == SCREEN_NPC_DIALOGUE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }

    game.teleportTo(MAP_DEYJA, Vec3f(4544, -12350, 2592), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_TAVERN_DEYJA);
    auto judge = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[78];
    });
    ASSERT_NE(judge, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", judge - houseNpcs.begin()));
        game.tick();
    }
    ASSERT_TRUE(selectNpcEvent(game, 78, pNPCStats->pNPCData[78].dialogue_1_evt_id));
    ASSERT_TRUE(pNPCStats->pNPCData[78].Hired());
    leaveHouse(game);

    game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_HARMONDALE_ARBITER_2);
    ASSERT_TRUE(pParty->_questBits[QBIT_DARK_PATH]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ARCHIBALD_VISIT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_ARBITER_CHOICE_ACTIVE]);
    ASSERT_FALSE(pNPCStats->pNPCData[78].Hired());
    ASSERT_EQ(pNPCStats->pNPCData[78].house, HOUSE_HARMONDALE_ARBITER_2);
    leaveHouse(game);
}

} // namespace

GAME_TEST(MM7WarRevealMatrix, SourceContracts) {
    game.startNewGame();
    const EvtProgram &global = engine->_globalEventMap;

    EXPECT_EQ(pNPCStats->pNPCData[catherineId].dialogue_2_evt_id, 126u);
    EXPECT_EQ(pNPCStats->pNPCData[parsonId].dialogue_2_evt_id, 133u);
    EXPECT_TRUE(eventUsesVariable(global, 126, 0, EVENT_OnCanShowDialogItemCmp, VAR_NPCs2,
                                  falseLorenId, 3));
    EXPECT_TRUE(eventUsesVariable(global, 133, 0, EVENT_OnCanShowDialogItemCmp,
                                  VAR_PlayerItemInHands,
                                  std::to_underlying(ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS), 3));
    EXPECT_TRUE(eventUsesVariable(global, 126, 10, EVENT_Set, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_FALSE_LOREN_REVEALED)));
    EXPECT_TRUE(eventUsesVariable(global, 133, 10, EVENT_Set, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED)));
    EXPECT_EQ(countGlobalVariableUses(EVENT_Set, VAR_QBits_QuestsDone,
                                      std::to_underlying(QBIT_FALSE_LOREN_REVEALED)), 1);
    EXPECT_EQ(countGlobalVariableUses(EVENT_Set, VAR_QBits_QuestsDone,
                                      std::to_underlying(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED)), 1);
    for (EvtOpcode opcode : {EVENT_Add, EVENT_Subtract}) {
        EXPECT_EQ(countGlobalVariableUses(opcode, VAR_QBits_QuestsDone,
                                          std::to_underlying(QBIT_FALSE_LOREN_REVEALED)), 0);
        EXPECT_EQ(countGlobalVariableUses(
                      opcode, VAR_QBits_QuestsDone,
                      std::to_underlying(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED)), 0);
    }

    EXPECT_TRUE(eventUsesVariable(global, 135, 1, EVENT_Compare, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_FALSE_LOREN_REVEALED), 13));
    EXPECT_TRUE(eventUsesVariable(global, 128, 1, EVENT_Compare, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED), 13));
    EXPECT_EQ(countGlobalVariableUses(EVENT_Compare, VAR_QBits_QuestsDone,
                                      std::to_underlying(QBIT_FALSE_LOREN_REVEALED)), 1);
    EXPECT_EQ(countGlobalVariableUses(EVENT_Compare, VAR_QBits_QuestsDone,
                                      std::to_underlying(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED)), 1);
    for (int eventId : {126, 133}) {
        EXPECT_EQ(std::ranges::count_if(global.function(eventId), [](const EvtInstruction &instruction) {
            return eventUsesVariable(instruction, EVENT_Set, VAR_Dead, 1);
        }), 4);
    }

    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(37));
    EXPECT_TRUE(std::ranges::none_of(engine->_localEventMap.function(37),
                                     [](const EvtInstruction &instruction) {
        return instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               (instruction.data.variable_descr.value ==
                    std::to_underlying(QBIT_FALSE_LOREN_REVEALED) ||
                instruction.data.variable_descr.value ==
                    std::to_underlying(QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED));
    }));
}

GAME_TEST(MM7WarAlternatives, LorenRealDecoyAndExpiredOutcomes) {
    for (WarJobOutcome outcome : {WarJobOutcome::REAL, WarJobOutcome::DECOY, WarJobOutcome::EXPIRED}) {
        SCOPED_TRACE(fmt::format("outcome={}", std::to_underlying(outcome)));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();
        const int initialExperience = totalExperience();
        openCatherine(game);
        const unsigned int offer = findNpcEvent(catherineId, [](const EvtInstruction &instruction) {
            return eventChangesQuestBit(instruction, QBIT_LOREN_STEEL_RESCUE_ACTIVE) &&
                   instruction.opcode != EVENT_Subtract;
        });
        ASSERT_TRUE(selectNpcEvent(game, catherineId, offer));
        ASSERT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUE_ACTIVE]);
        ASSERT_TRUE(pParty->_questBits[QBIT_TALKED_TO_CATHERINE]);
        leaveHouse(game);

        if (outcome == WarJobOutcome::REAL) {
            acquireLoren(game);
        } else if (outcome == WarJobOutcome::DECOY) {
            openParson(game);
            const unsigned int decoy = findNpcEvent(parsonId, [](const EvtInstruction &instruction) {
                return eventChangesQuestBit(instruction, QBIT_FALSE_LOREN_ACTIVE) &&
                       instruction.opcode != EVENT_Subtract;
            });
            ASSERT_TRUE(selectNpcEvent(game, parsonId, decoy));
            ASSERT_TRUE(pParty->_questBits[QBIT_FALSE_LOREN_ACTIVE]);
            ASSERT_TRUE(pNPCStats->pNPCData[falseLorenId].Hired());
            leaveHouse(game);
        } else {
            pParty->GetPlayingTime() += Duration::fromDays(29);
        }

        openCatherine(game);
        const unsigned int completion = pNPCStats->pNPCData[catherineId].dialogue_1_evt_id;
        ASSERT_TRUE(selectNpcEvent(game, catherineId, completion));
        EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUE_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_ACTIVE]);
        EXPECT_FALSE(pNPCStats->pNPCData[lorenId].Hired());
        EXPECT_FALSE(pNPCStats->pNPCData[falseLorenId].Hired());

        if (outcome == WarJobOutcome::REAL) {
            EXPECT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 6000);
            EXPECT_EQ(totalExperience(), initialExperience + 40000);
        } else if (outcome == WarJobOutcome::DECOY) {
            EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
            EXPECT_TRUE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        } else {
            EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        }
        for (const Character &character : pParty->pCharacters) {
            EXPECT_EQ(character._achievedAwardsBits[AWARD_ERATHIA_RESCUED_LOREN_STEEL],
                      outcome == WarJobOutcome::REAL);
            EXPECT_EQ(character._achievedAwardsBits[AWARD_AVLEE_BROUGHT_FAKE_LOREN_STEEL_TO_ERATHIA],
                      outcome == WarJobOutcome::DECOY);
        }
    }
}

GAME_TEST(MM7WarAlternatives, RiverstrideRealDecoyAndExpiredOutcomes) {
    for (WarJobOutcome outcome : {WarJobOutcome::REAL, WarJobOutcome::DECOY, WarJobOutcome::EXPIRED}) {
        SCOPED_TRACE(fmt::format("outcome={}", std::to_underlying(outcome)));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();
        const int initialExperience = totalExperience();
        openParson(game);
        const unsigned int offer = findNpcEvent(parsonId, [](const EvtInstruction &instruction) {
            return eventChangesQuestBit(instruction, QBIT_RIVERSTRIDE_PLANS_ACTIVE) &&
                   instruction.opcode != EVENT_Subtract;
        });
        ASSERT_TRUE(selectNpcEvent(game, parsonId, offer));
        ASSERT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_ACTIVE]);
        ASSERT_TRUE(pParty->_questBits[QBIT_TALKED_TO_ELF_KING]);
        leaveHouse(game);

        if (outcome == WarJobOutcome::REAL) {
            acquireRiverstridePlans(game);
        } else if (outcome == WarJobOutcome::DECOY) {
            openCatherine(game);
            const unsigned int decoy = findNpcEvent(catherineId, [](const EvtInstruction &instruction) {
                return eventChangesQuestBit(instruction, QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE) &&
                       instruction.opcode != EVENT_Subtract;
            });
            ASSERT_TRUE(selectNpcEvent(game, catherineId, decoy));
            ASSERT_TRUE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE]);
            ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS);
            pParty->placeHeldItemInInventoryOrDrop();
            leaveHouse(game);
        } else {
            pParty->GetPlayingTime() += Duration::fromDays(29);
        }

        openParson(game);
        const unsigned int completion = pNPCStats->pNPCData[parsonId].dialogue_1_evt_id;
        ASSERT_TRUE(selectNpcEvent(game, parsonId, completion));
        EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE]);
        EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS));
        EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS));

        if (outcome == WarJobOutcome::REAL) {
            EXPECT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 6000);
            EXPECT_EQ(totalExperience(), initialExperience + 40000);
        } else if (outcome == WarJobOutcome::DECOY) {
            EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_TRUE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        } else {
            EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        }
        for (const Character &character : pParty->pCharacters) {
            EXPECT_EQ(character._achievedAwardsBits[AWARD_AVLEE_BROUGHT_FORT_RIVERSTRIDE_PLANS],
                      outcome == WarJobOutcome::REAL);
            EXPECT_EQ(character._achievedAwardsBits[AWARD_ERATHIA_DELIVERED_FAKE_PLANS_TO_ELVES],
                      outcome == WarJobOutcome::DECOY);
        }
    }
}

GAME_TEST(MM7WarRevealMatrix, ExposedDecoysArePhysicalDeathAndFailureRoutes) {
    for (bool exposeFalseLoren : {true, false}) {
        SCOPED_TRACE(fmt::format("exposeFalseLoren={}", exposeFalseLoren));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();

        int targetNpcId;
        int revealEventId;
        int sponsorNpcId;
        int sponsorFollowupEventId;
        QuestBit revealBit;
        if (exposeFalseLoren) {
            openCatherine(game);
            ASSERT_TRUE(selectNpcEvent(game, catherineId, 124));
            leaveHouse(game);
            openParson(game);
            ASSERT_TRUE(selectNpcEvent(game, parsonId, 134));
            ASSERT_TRUE(pNPCStats->pNPCData[falseLorenId].Hired());
            ASSERT_TRUE(pParty->_questBits[QBIT_FALSE_LOREN_ACTIVE]);
            leaveHouse(game);

            openCatherine(game);
            targetNpcId = catherineId;
            revealEventId = 126;
            sponsorNpcId = parsonId;
            sponsorFollowupEventId = 135;
            revealBit = QBIT_FALSE_LOREN_REVEALED;
        } else {
            openParson(game);
            ASSERT_TRUE(selectNpcEvent(game, parsonId, 131));
            leaveHouse(game);
            openCatherine(game);
            ASSERT_TRUE(selectNpcEvent(game, catherineId, 127));
            ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS);
            pParty->placeHeldItemInInventoryOrDrop();
            ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS));
            ASSERT_TRUE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE]);
            leaveHouse(game);

            openParson(game);
            targetNpcId = parsonId;
            revealEventId = 133;
            sponsorNpcId = catherineId;
            sponsorFollowupEventId = 128;
            revealBit = QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED;
        }

        const auto topics = prepareScriptedNPCDialogueTopics(&pNPCStats->pNPCData[targetNpcId]);
        ASSERT_TRUE(std::ranges::contains(topics, DIALOGUE_SCRIPTED_LINE_2));
        const int deathsBefore = pParty->uNumDeaths;
        ASSERT_TRUE(selectNpcEvent(game, targetNpcId, revealEventId));
        for (const Character &character : pParty->pCharacters)
            EXPECT_TRUE(character.conditions.has(CONDITION_DEAD));
        leaveHouse(game);
        waitForPartyDeathRecovery(game, deathsBefore);

        EXPECT_TRUE(pParty->_questBits[revealBit]);
        EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUE_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
        EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
        EXPECT_FALSE(pNPCStats->pNPCData[lorenId].Hired());
        EXPECT_FALSE(pNPCStats->pNPCData[falseLorenId].Hired());
        EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS));
        EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS));
        EXPECT_EQ(pNPCStats->pNPCData[targetNpcId].dialogue_1_evt_id, 0u);
        EXPECT_EQ(pNPCStats->pNPCData[targetNpcId].dialogue_2_evt_id, 0u);
        EXPECT_FALSE(selectNpcEvent(game, targetNpcId, revealEventId));

        if (sponsorNpcId == catherineId)
            openCatherine(game);
        else
            openParson(game);
        const int goldBefore = pParty->GetGold();
        const int bankBefore = pParty->uNumGoldInBank;
        const int experienceBefore = totalExperience();
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, sponsorNpcId, sponsorFollowupEventId));
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->uNumGoldInBank, bankBefore);
        EXPECT_EQ(totalExperience(), experienceBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        EXPECT_TRUE(pParty->_questBits[revealBit]);
        EXPECT_EQ(pNPCStats->pNPCData[sponsorNpcId].dialogue_3_evt_id, 0u);
        EXPECT_EQ(pNPCStats->pNPCData[sponsorNpcId].dialogue_4_evt_id, 0u);
        EXPECT_FALSE(selectNpcEvent(game, sponsorNpcId, sponsorFollowupEventId));
    }
}

GAME_TEST(MM7WarAlternatives, TrumpetSupportsErathiaAvleeAndArbiterRecipients) {
    for (TrumpetRecipient recipient : {TrumpetRecipient::CATHERINE, TrumpetRecipient::PARSON,
                                       TrumpetRecipient::ARBITER}) {
        SCOPED_TRACE(fmt::format("recipient={}", std::to_underlying(recipient)));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();
        const int initialExperience = totalExperience();
        acquireGryphonheartTrumpet(game);

        int npcId;
        QuestBit outcomeBit;
        if (recipient == TrumpetRecipient::CATHERINE) {
            openCatherine(game);
            npcId = catherineId;
            outcomeBit = QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA;
        } else if (recipient == TrumpetRecipient::PARSON) {
            openParson(game);
            npcId = parsonId;
            outcomeBit = QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE;
        } else {
            openJudgeGrey(game);
            npcId = 74;
            outcomeBit = QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER;
        }
        const unsigned int handIn = findNpcEvent(npcId, [outcomeBit](const EvtInstruction &instruction) {
            return eventChangesQuestBit(instruction, outcomeBit) && instruction.opcode != EVENT_Subtract;
        });
        ASSERT_TRUE(selectNpcEvent(game, npcId, handIn));

        EXPECT_FALSE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_ACTIVE]);
        EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_GRYPHONHEARTS_TRUMPET));
        EXPECT_EQ(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA],
                  recipient == TrumpetRecipient::CATHERINE);
        EXPECT_EQ(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE],
                  recipient == TrumpetRecipient::PARSON);
        EXPECT_EQ(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER],
                  recipient == TrumpetRecipient::ARBITER);
        EXPECT_EQ(pParty->uNumGoldInBank, recipient == TrumpetRecipient::ARBITER ? 1000 : 6000);
        EXPECT_EQ(pParty->GetGold(), 1000);
        EXPECT_EQ(totalExperience(), initialExperience + (recipient == TrumpetRecipient::ARBITER ? 50000 : 40000));
    }
}

GAME_TEST(MM7WarRevealMatrix, AllFivePhysicalTrumpetRecipientsHaveExactRewards) {
    struct RecipientCase {
        int npcId;
        int eventId;
        QuestBit recipientBit;
        int goldDelta;
        int bankDelta;
        int reputationDelta;
        int experienceDelta;
    };
    constexpr std::array cases = {
        RecipientCase{humanAmbassadorId, 118, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA,
                      5000, 0, 0, 40000},
        RecipientCase{catherineId, 130, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA,
                      0, 5000, -5, 40000},
        RecipientCase{elvenAmbassadorId, 122, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE,
                      5000, 0, 0, 40000},
        RecipientCase{parsonId, 137, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE,
                      0, 5000, -5, 40000},
        RecipientCase{judgeGreyId, 140, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER,
                      0, 0, 0, 50000},
    };

    for (const RecipientCase &testCase : cases) {
        SCOPED_TRACE(fmt::format("npc={}", testCase.npcId));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();
        acquireGryphonheartTrumpet(game);

        if (testCase.npcId == humanAmbassadorId || testCase.npcId == elvenAmbassadorId) {
            // Authored global event 108 places both ambassadors here after the castle rebuild.
            pNPCStats->pNPCData[testCase.npcId].house = HOUSE_CASTLE_HARMONDALE_THRONE_ROOM;
            openThroneRoom(game, MAP_CASTLE_HARMONDALE, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM,
                           testCase.npcId);
        } else if (testCase.npcId == catherineId) {
            openCatherine(game);
        } else if (testCase.npcId == parsonId) {
            openParson(game);
        } else {
            openJudgeGrey(game);
        }

        const int goldBefore = pParty->GetGold();
        const int bankBefore = pParty->uNumGoldInBank;
        const int reputationBefore = pParty->GetPartyReputation();
        const int experienceBefore = totalExperience();
        ASSERT_TRUE(selectNpcEvent(game, testCase.npcId, testCase.eventId));

        EXPECT_EQ(pParty->GetGold() - goldBefore, testCase.goldDelta);
        EXPECT_EQ(pParty->uNumGoldInBank - bankBefore, testCase.bankDelta);
        EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, testCase.reputationDelta);
        EXPECT_EQ(totalExperience() - experienceBefore, testCase.experienceDelta);
        EXPECT_TRUE(pParty->_questBits[testCase.recipientBit]);
        EXPECT_EQ(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA],
                  testCase.recipientBit == QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA);
        EXPECT_EQ(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE],
                  testCase.recipientBit == QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE);
        EXPECT_EQ(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER],
                  testCase.recipientBit == QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER);
        EXPECT_FALSE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_ACTIVE]);
        EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_GRYPHONHEARTS_TRUMPET));
        EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
        EXPECT_FALSE(selectNpcEvent(game, testCase.npcId, testCase.eventId));

        const int goldAfter = pParty->GetGold();
        const int bankAfter = pParty->uNumGoldInBank;
        const int reputationAfter = pParty->GetPartyReputation();
        const int experienceAfter = totalExperience();
        game.tick();
        EXPECT_EQ(pParty->GetGold(), goldAfter);
        EXPECT_EQ(pParty->uNumGoldInBank, bankAfter);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationAfter);
        EXPECT_EQ(totalExperience(), experienceAfter);
    }
}

GAME_TEST(MM7WarRevealMatrix, AllEffectiveResultsResolveUnderBothArbiters) {
    constexpr std::array resultStates = {
        WarResultState::NONE,
        WarResultState::REAL,
        WarResultState::FAKE,
    };
    constexpr std::array recipients = {
        TrumpetRecipient::CATHERINE,
        TrumpetRecipient::PARSON,
        TrumpetRecipient::ARBITER,
    };
    constexpr std::array paths = {
        ArbiterPath::LIGHT,
        ArbiterPath::DARK,
    };

    for (WarResultState plans : resultStates) {
        for (WarResultState loren : resultStates) {
            for (TrumpetRecipient recipient : recipients) {
                for (ArbiterPath path : paths) {
                    SCOPED_TRACE(fmt::format(
                        "plans={} loren={} recipient={} path={} (NONE=exposed/expired equivalent)",
                        std::to_underlying(plans), std::to_underlying(loren),
                        std::to_underlying(recipient), std::to_underlying(path)));
                    test.prepareForNextTest();
                    game.startNewGame();
                    pParty->_questBits.reset();
                    pParty->_questBits.set(QBIT_HARMONDALE_REBUILT);
                    pParty->_questBits.set(QBIT_ARBITER_CHOICE_ACTIVE);
                    setWarResultBits(plans, loren, recipient);

                    const bool light = path == ArbiterPath::LIGHT;
                    NPCData &judge = pNPCStats->pNPCData[light ? goodJudgeId : evilJudgeId];
                    NPCData &otherJudge = pNPCStats->pNPCData[light ? evilJudgeId : goodJudgeId];
                    judge.flags |= NPC_HIRED;
                    judge.dialogue_1_evt_id = 0;  // State after authored hire event 143/141.
                    otherJudge.flags &= ~NPC_HIRED;

                    loadMapAtPartyStart(game, MAP_HARMONDALE);
                    game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90);
                    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                    game.tick();
                    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
                    const WarWinner winner = expectedWarWinner(plans, loren, recipient);
                    const int historyBase = light ? 6 : 13;
                    const int winnerOffset = winner == WarWinner::HUMAN ? 0 :
                                             winner == WarWinner::ELF ? 1 : 2;
                    for (int offset = 0; offset < 3; ++offset) {
                        EXPECT_EQ(pParty->PartyTimes.HistoryEventTimes[historyBase + offset].isValid(),
                                  offset == winnerOffset);
                    }

                    EXPECT_EQ(pParty->_questBits[QBIT_LIGHT_PATH], light);
                    EXPECT_EQ(pParty->_questBits[QBIT_DARK_PATH], !light);
                    EXPECT_EQ(pParty->_questBits[QBIT_GAVIN_MAGNUS_VISIT_ACTIVE], light);
                    EXPECT_EQ(pParty->_questBits[QBIT_ARCHIBALD_VISIT_ACTIVE], !light);
                    EXPECT_FALSE(pParty->_questBits[QBIT_ARBITER_CHOICE_ACTIVE]);
                    EXPECT_FALSE(judge.Hired());
                    EXPECT_FALSE(otherJudge.Hired());
                    EXPECT_EQ(judge.house, light ? HOUSE_HARMONDALE_ARBITER_1
                                                 : HOUSE_HARMONDALE_ARBITER_2);
                    EXPECT_EQ(otherJudge.house, HOUSE_INVALID);
                    EXPECT_EQ(judge.greetingIndex, light ? 106 : 103);
                    EXPECT_EQ(judge.dialogue_2_evt_id, light ? 144u : 142u);
                    EXPECT_EQ(judge.dialogue_3_evt_id, 139u);

                    EXPECT_EQ(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_REVEALED],
                              plans == WarResultState::NONE);
                    EXPECT_EQ(pParty->_questBits[QBIT_FALSE_LOREN_REVEALED],
                              loren == WarResultState::NONE);
                }
            }
        }
    }
}

GAME_TEST(MM7WarAlternatives, BalancedWarAndNeutralTrumpetCanInstallDarkArbiter) {
    game.startNewGame();
    prepareWarJobs();
    installDarkArbiterAfterBalancedWar(game);

    EXPECT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
    EXPECT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER]);
    EXPECT_FALSE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA]);
    EXPECT_FALSE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE]);
    EXPECT_EQ(pParty->alignment, PartyAlignment::PartyAlignment_Evil);
}

GAME_TEST(MM7WarCheckpoint, DISABLED_WriteNeutralDarkArbiter) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    game.startNewGame();
    prepareWarJobs();
    installDarkArbiterAfterBalancedWar(game);
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Neutral war, Dark Arbiter checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7WarCheckpoint, DISABLED_ReloadNeutralDarkArbiter) {
    assertDisposableHeadlessProfile();
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
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
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
    EXPECT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ARBITER_MESSENGER_MET]);
    EXPECT_TRUE(pParty->_questBits[QBIT_DARK_PATH]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ARCHIBALD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ARBITER_CHOICE_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_GRYPHONHEARTS_TRUMPET));
    EXPECT_EQ(pParty->alignment, PartyAlignment::PartyAlignment_Evil);
    EXPECT_EQ(pParty->GetGold(), 1000);
    EXPECT_EQ(pParty->uNumGoldInBank, 1000);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperience[i]);
    EXPECT_TRUE(pParty->PartyTimes.CounterEventValues[2].isValid());
    EXPECT_GE(pParty->GetPlayingTime(), pParty->PartyTimes.CounterEventValues[2] + Duration::fromDays(112));
    EXPECT_FALSE(pNPCStats->pNPCData[78].Hired());
    EXPECT_EQ(pNPCStats->pNPCData[78].house, HOUSE_HARMONDALE_ARBITER_2);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
