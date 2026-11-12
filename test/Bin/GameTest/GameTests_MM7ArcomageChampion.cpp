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

#include "Arcomage/Arcomage.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnumFunctions.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/HouseTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {7000, 7100, 7200, 7300};
constexpr int checkpointGoldBaseline = 8000;

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventSetsQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

NPCData *findGinaBarnes() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array eventIds = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                               candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return std::ranges::any_of(eventIds, [](unsigned int eventId) {
            return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                   std::ranges::any_of(engine->_globalEventMap.function(eventId), [](const EvtInstruction &instruction) {
                       return eventSetsQuestBit(instruction, QBIT_ARCOMAGE_ALL_TAVERNS_ACTIVE);
                   });
        });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int findNpcEvent(const NPCData *npc, auto predicate) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void openGinaHouse(EngineController &game, NPCData *gina) {
    game.teleportTo(MAP_ERATHIA, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, gina->house))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_ERATHIA, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == gina->house)
            break;
        if (current_screen_type == SCREEN_NPC_DIALOGUE) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), gina->house);
    auto entry = std::ranges::find(houseNpcs, gina, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

int arcomageTavernGold() {
    int result = 0;
    for (HouseId tavern : allArcomageTaverns())
        result += houseTable[tavern].fPriceMultiplier * 100;
    return result;
}

int rewardChestId() {
    for (const BLVFace &face : pOutdoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        for (const EvtInstruction &instruction : engine->_localEventMap.function(face.eventId)) {
            if (instruction.opcode == EVENT_OpenChest && instruction.data.chest_id >= 0 &&
                instruction.data.chest_id < vChests.size() &&
                vChests[instruction.data.chest_id].inventory.find(ITEM_ARTIFACT_FORGE_GAUNTLETS)) {
                return instruction.data.chest_id;
            }
        }
    }
    return -1;
}

void lootRewardChest(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_ERATHIA);
    const int chestId = rewardChestId();
    ASSERT_NE(chestId, -1);
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(chestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    InventoryEntry gauntlets = vChests[chestId].inventory.find(ITEM_ARTIFACT_FORGE_GAUNTLETS);
    ASSERT_TRUE(gauntlets);
    Pointi position = chestTable[vChests[chestId].chestTypeId].inventoryOffset +
                      gauntlets.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, position);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_ARTIFACT_FORGE_GAUNTLETS);
    pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_ARTIFACT_FORGE_GAUNTLETS));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
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

void runArcomageChampionRoute(EngineController &game, bool useCheckpointBaseline = false) {
    game.startNewGame();
    if (useCheckpointBaseline) {
        pParty->SetGold(checkpointGoldBaseline);
        for (int i = 0; i < 4; ++i)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }
    NPCData *gina = findGinaBarnes();
    ASSERT_NE(gina, nullptr);
    const unsigned int offerEvent = findNpcEvent(gina, [](const EvtInstruction &instruction) {
        return eventSetsQuestBit(instruction, QBIT_ARCOMAGE_ALL_TAVERNS_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);
    const int initialGold = pParty->uNumGold;
    const int initialExperience = totalExperience();

    openGinaHouse(game, gina);
    ASSERT_TRUE(selectNpcEvent(game, gina, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_ARCOMAGE_ALL_TAVERNS_ACTIVE]);
    const unsigned int handInEvent = findNpcEvent(gina, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_ARCOMAGE_ALL_TAVERNS_ACTIVE);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);

    openGinaHouse(game, gina);
    ASSERT_TRUE(selectNpcEvent(game, gina, handInEvent));
    EXPECT_FALSE(pParty->_questBits[QBIT_ARCOMAGE_QUEST_FINISHED]);
    EXPECT_EQ(pParty->uNumGold, initialGold);
    EXPECT_EQ(totalExperience(), initialExperience);
    leaveHouse(game);

    int completed = 0;
    for (HouseId tavern : allArcomageTaverns()) {
        EXPECT_TRUE(isArcomageTavern(tavern));
        EXPECT_EQ(houseTable[tavern].uType, HOUSE_TYPE_TAVERN);
        recordArcomageGameResult(tavern, true);
        EXPECT_TRUE(pParty->pArcomageWins[tavern]);
        ++completed;
        EXPECT_EQ(pParty->_questBits[QBIT_ARCOMAGE_CHAMPION], completed == allArcomageTaverns().size());
    }
    EXPECT_EQ(completed, 13);
    EXPECT_EQ(pParty->uNumArcomageWins, 13);
    EXPECT_EQ(pParty->uNumGold, initialGold + arcomageTavernGold());
    for (const Character &character : pParty->pCharacters)
        EXPECT_TRUE(character._achievedAwardsBits[AWARD_ARCOMAGE_WINS]);

    const int goldAfterFirstWins = pParty->uNumGold;
    recordArcomageGameResult(HOUSE_TAVERN_HARMONDALE, true);
    EXPECT_EQ(pParty->uNumGold, goldAfterFirstWins);
    EXPECT_EQ(pParty->uNumArcomageWins, 14);

    openGinaHouse(game, gina);
    ASSERT_TRUE(selectNpcEvent(game, gina, handInEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_ARCOMAGE_QUEST_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ARCOMAGE_ALL_TAVERNS_ACTIVE]);
    EXPECT_EQ(pParty->uNumGold, initialGold + arcomageTavernGold() + 100000);
    EXPECT_EQ(totalExperience(), initialExperience + 4 * 50000);
    for (const Character &character : pParty->pCharacters)
        EXPECT_TRUE(character._achievedAwardsBits[AWARD_ARCOMAGE_CHAMPION]);
    leaveHouse(game);
    lootRewardChest(game);
}

} // namespace

GAME_TEST(MM7SideQuests, ArcomageChampionRequiresAllTavernResultsAndGina) {
    runArcomageChampionRoute(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteArcomageChampion) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runArcomageChampionRoute(game, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_ERATHIA);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Arcomage Champion checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadArcomageChampion) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_ERATHIA);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_ARCOMAGE_CHAMPION]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ARCOMAGE_QUEST_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ARCOMAGE_ALL_TAVERNS_ACTIVE]);
    for (HouseId tavern : allArcomageTaverns())
        EXPECT_TRUE(pParty->pArcomageWins[tavern]);
    EXPECT_EQ(pParty->uNumArcomageWins, 14);
    EXPECT_EQ(pParty->uNumGold, checkpointGoldBaseline + arcomageTavernGold() + 100000);
    EXPECT_TRUE(pParty->hasItem(ITEM_ARTIFACT_FORGE_GAUNTLETS));
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperienceBaseline[i] + 50000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_ARCOMAGE_WINS]);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_ARCOMAGE_CHAMPION]);
    }
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
