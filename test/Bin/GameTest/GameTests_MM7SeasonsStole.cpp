#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {1000, 1100, 1200, 1300};

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool opensZimmHouse(int eventId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse &&
                      instruction.data.house_id == HOUSE_BRACADA_DESERT_ZIMMS_HOUSE;
           });
}

NPCData *findGaryZimm() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array events = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                             candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == HOUSE_BRACADA_DESERT_ZIMMS_HOUSE &&
               std::ranges::any_of(events, [](unsigned int eventId) {
                   return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                          std::ranges::any_of(engine->_globalEventMap.function(eventId),
                                              [](const EvtInstruction &instruction) {
                                                  return (instruction.opcode == EVENT_Set ||
                                                          instruction.opcode == EVENT_Add) &&
                                                         instruction.data.variable_descr.type ==
                                                             VAR_QBits_QuestsDone &&
                                                         instruction.data.variable_descr.value ==
                                                             std::to_underlying(QBIT_SEASONS_STOLE_ACTIVE);
                                              });
               });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int findNpcEvent(const NPCData *npc, auto predicate) {
    std::array events = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                         npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find_if(events, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, events.end());
    return event == events.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array events = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                         npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(events, eventId);
    if (event == events.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - events.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void openZimmHouse(EngineController &game, NPCData *gary) {
    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces)
        if (face.Clickable() && opensZimmHouse(face.eventId))
            doors.push_back(approachFace(face));
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_BRACADA_DESERT, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE &&
            window_SpeakInHouse->houseId() == HOUSE_BRACADA_DESERT_ZIMMS_HOUSE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto entry = std::ranges::find(houseNpcs, gary, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveUi(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void runSeasonsStoleRoute(EngineController &game, TestController &test, CommonTapeRecorder &tapes,
                          bool useCheckpointExperienceBaseline = false) {
    auto activeTape = tapes.questBit(QBIT_SEASONS_STOLE_ACTIVE);
    auto goldTape = tapes.gold();
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    if (useCheckpointExperienceBaseline) {
        for (int i = 0; i < 4; i++)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }
    pParty->_questBits.set(QBIT_LIGHT_PATH); // Gary offers this regional task only to a Light-path party.
    NPCData *gary = findGaryZimm();
    ASSERT_NE(gary, nullptr);
    unsigned int offerEvent = findNpcEvent(gary, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_SEASONS_STOLE_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);

    test.startTaping();
    openZimmHouse(game, gary);
    ASSERT_TRUE(selectNpcEvent(game, gary, offerEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_SEASONS_STOLE_ACTIVE]);
    unsigned int handInEvent = findNpcEvent(gary, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_SEASONS_STOLE);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveUi(game);

    openZimmHouse(game, gary);
    ASSERT_TRUE(selectNpcEvent(game, gary, handInEvent)); // The return topic refuses the missing Stole.
    EXPECT_TRUE(pParty->_questBits[QBIT_SEASONS_STOLE_ACTIVE]);
    EXPECT_EQ(goldTape.delta(), 0);
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_BRACADA_RETRIEVED_SEASONS_STOLE]);
    leaveUi(game);

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
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_SEASONS_STOLE) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_SEASONS_STOLE);
    leaveUi(game);

    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; i++)
        experience[i] = pParty->pCharacters[i].experience;
    const int activeCharacter = pParty->activeCharacterIndex();
    openZimmHouse(game, gary);
    ASSERT_TRUE(selectNpcEvent(game, gary, handInEvent));
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_SEASONS_STOLE));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(goldTape.delta(), 7500);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits[AWARD_BRACADA_RETRIEVED_SEASONS_STOLE],
                  i == activeCharacter);
        EXPECT_EQ(pParty->pCharacters[i].experience - experience[i], 7500);
    }
}

} // namespace

GAME_TEST(MM7SideQuests, SeasonsStoleRequiresHallOfThePitChest) {
    runSeasonsStoleRoute(game, test, tapes);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteSeasonsStole) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runSeasonsStoleRoute(game, test, tapes, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    leaveUi(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BRACADA_DESERT);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Season's Stole regional checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadSeasonsStole) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BRACADA_DESERT);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    EXPECT_FALSE(pParty->_questBits[QBIT_SEASONS_STOLE_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_SEASONS_STOLE));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(pParty->uNumGold, engine->config->gameplay.NewGameGold.value() + 7500);
    const int activeCharacter = pParty->activeCharacterIndex();
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits[AWARD_BRACADA_RETRIEVED_SEASONS_STOLE],
                  i == activeCharacter);
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperienceBaseline[i] + 7500);
    }
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
