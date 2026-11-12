#include <algorithm>
#include <array>
#include <filesystem>
#include <span>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/SpriteObject.h"
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

#include "Utility/Exception.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {4000, 4100, 4200, 4300};
constexpr int checkpointGoldBaseline = 5000;

bool setsMissingContestantsQuest(const EvtInstruction &instruction) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(QBIT_EMERALD_ISLAND_MISSING_CONTESTANTS_ACTIVE);
}

NPCData *findMarkham() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array events = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                             candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == HOUSE_EMERALD_ISLAND_MARKHAMS_HEADQUARTERS &&
               std::ranges::any_of(events, [](unsigned int eventId) {
                   return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                          std::ranges::any_of(engine->_globalEventMap.function(eventId), setsMissingContestantsQuest);
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

void openMarkhamHeadquarters(EngineController &game, NPCData *markham) {
    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(16154, 8560, 128), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_EMERALD_ISLAND_MARKHAMS_HEADQUARTERS);
    auto entry = std::ranges::find(houseNpcs, markham, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

int contestantShieldSprite() {
    auto sprite = std::ranges::find_if(pSpriteObjects, [](const SpriteObject &object) {
        return object.containing_item.itemId == ITEM_QUEST_CONTESTANTS_SHIELD;
    });
    return sprite == pSpriteObjects.end() ? -1 : sprite - pSpriteObjects.begin();
}

bool pickUpContestantShield(EngineController &game, int spriteId) {
    const Vec3f position = pSpriteObjects[spriteId].vPosition;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -200, 0), 90), std::pair(Vec3f(-200, 0, 0), 0),
                               std::pair(Vec3f(0, 200, 0), 270), std::pair(Vec3f(200, 0, 0), 180)}) {
        game.teleportTo(MAP_DRAGONS_LAIR, position + offset, yaw, -10);
        try {
            game.pointMouseAtSprite(spriteId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        if (pParty->hasItem(ITEM_QUEST_CONTESTANTS_SHIELD) ||
            pParty->pPickedItem.itemId == ITEM_QUEST_CONTESTANTS_SHIELD)
            return true;
    }
    return false;
}

bool acquireContestantShield(EngineController &game) {
    const int sprite = contestantShieldSprite();
    return sprite >= 0 && pickUpContestantShield(game, sprite);
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void runMissingContestantsRoute(EngineController &game, TestController &test, CommonTapeRecorder &tapes,
                                bool useCheckpointBaseline = false) {
    auto activeTape = tapes.questBit(QBIT_EMERALD_ISLAND_MISSING_CONTESTANTS_ACTIVE);
    auto goldTape = tapes.gold();
    engine->config->debug.NoDamage.setValue(true);
    engine->config->debug.NoMargaret.setValue(true);
    game.startNewGame();
    if (useCheckpointBaseline) {
        pParty->SetGold(checkpointGoldBaseline);
        for (int i = 0; i < 4; i++)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }

    NPCData *markham = findMarkham();
    ASSERT_NE(markham, nullptr);
    const unsigned int offerEvent = findNpcEvent(markham, setsMissingContestantsQuest);
    ASSERT_NE(offerEvent, 0u);

    test.startTaping();
    openMarkhamHeadquarters(game, markham);
    ASSERT_TRUE(selectNpcEvent(game, markham, offerEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_MISSING_CONTESTANTS_ACTIVE]);
    const unsigned int handInEvent = findNpcEvent(markham, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_CONTESTANTS_SHIELD);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);

    openMarkhamHeadquarters(game, markham);
    ASSERT_TRUE(selectNpcEvent(game, markham, handInEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_MISSING_CONTESTANTS_ACTIVE]);
    EXPECT_EQ(goldTape.delta(), 0);
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_EMERALD_ISLE_FOUND_MISSING_COMPETITORS]);
    leaveHouse(game);

    game.teleportTo(MAP_DRAGONS_LAIR, Vec3f(), 0);
    ASSERT_TRUE(acquireContestantShield(game));
    EXPECT_EQ(contestantShieldSprite(), -1);

    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; i++)
        experience[i] = pParty->pCharacters[i].experience;
    openMarkhamHeadquarters(game, markham);
    ASSERT_TRUE(selectNpcEvent(game, markham, handInEvent));
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_CONTESTANTS_SHIELD));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(goldTape.delta(), 1000);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_EMERALD_ISLE_FOUND_MISSING_COMPETITORS]);
        EXPECT_EQ(pParty->pCharacters[i].experience - experience[i], 1000);
    }
}

} // namespace

GAME_TEST(MM7SideQuests, MissingContestantsRequiresDragonCaveShield) {
    runMissingContestantsRoute(game, test, tapes);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteMissingContestants) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runMissingContestantsRoute(game, test, tapes, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    leaveHouse(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Missing Contestants checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadMissingContestants) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_MISSING_CONTESTANTS_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_CONTESTANTS_SHIELD));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(pParty->uNumGold, checkpointGoldBaseline + 1000);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_EMERALD_ISLE_FOUND_MISSING_COMPETITORS]);
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperienceBaseline[i] + 1000);
    }
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
