#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
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
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
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

constexpr Vec3f slicersHouseApproach = Vec3f(-19150, 14456, 0);
constexpr unsigned int griffinReportEvent = 197;
constexpr int checkpointGold = 1234;
constexpr int checkpointBankGold = 4321;
constexpr int checkpointReputation = 25;
constexpr int checkpointActiveCharacter = 0;
constexpr std::array<uint64_t, 4> checkpointExperience = {2000, 2100, 2200, 2300};
constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

bool changesVariable(const EvtInstruction &instruction, EvtOpcode opcode, EvtVariable variable, int value) {
    return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
           instruction.data.variable_descr.value == value;
}

bool eventContains(unsigned int eventId, auto predicate) {
    return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
}

std::array<unsigned int, 6> npcEvents(const NPCData *npc) {
    return {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
            npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
}

unsigned int findNpcEvent(const NPCData *npc, auto predicate) {
    const std::array events = npcEvents(npc);
    auto event = std::ranges::find_if(events, [predicate](unsigned int eventId) {
        return eventContains(eventId, predicate);
    });
    EXPECT_NE(event, events.end());
    return event == events.end() ? 0 : *event;
}

DialogueId dialogueForEvent(const NPCData *npc, unsigned int eventId) {
    const std::array events = npcEvents(npc);
    auto event = std::ranges::find(events, eventId);
    EXPECT_NE(event, events.end());
    if (event == events.end())
        return DIALOGUE_NULL;
    return static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - events.begin()));
}

bool isNpcEventVisible(NPCData *npc, unsigned int eventId) {
    const DialogueId dialogue = dialogueForEvent(npc, eventId);
    const std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    return std::ranges::find(topics, dialogue) != topics.end();
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    const DialogueId dialogue = dialogueForEvent(npc, eventId);
    const std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, dialogue);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

NPCData *findSethDrakkson() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        if (candidate.house != HOUSE_DEYJA_SLICERS_HOUSE)
            return false;
        return std::ranges::any_of(npcEvents(&candidate), [](unsigned int eventId) {
            return eventContains(eventId, [](const EvtInstruction &instruction) {
                return changesVariable(instruction, EVENT_Set, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_GRIFFINS_ACTIVE)) ||
                       changesVariable(instruction, EVENT_Add, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_GRIFFINS_ACTIVE));
            });
        });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

NPCData *findSethWithReportTopic() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        return candidate.house == HOUSE_DEYJA_SLICERS_HOUSE &&
               std::ranges::contains(npcEvents(&candidate), griffinReportEvent);
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

void openSethDialogue(EngineController &game, NPCData *seth) {
    // Setup teleport only; the authored viewport face opens Slicer's house.
    game.teleportTo(MAP_DEYJA, slicersHouseApproach, 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_NE(window_SpeakInHouse, nullptr);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_DEYJA_SLICERS_HOUSE);

    auto entry = std::ranges::find(houseNpcs, seth, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
}

void leaveHouse(EngineController &game) {
    for (int attempt = 0; attempt < 4 && current_screen_type != SCREEN_GAME; ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

int fixtureRegionalGriffins(EngineController &game, MapId map, bool leaveOneAlive) {
    game.teleportTo(map, Vec3f(), 0);
    int griffinCount = 0;
    int survivorId = -1;
    for (Actor &actor : pActors) {
        if (monsterTypeForMonsterId(actor.monsterInfo.id) != MONSTER_TYPE_GRIFFIN)
            continue;
        ++griffinCount;
        if (leaveOneAlive && survivorId == -1) {
            survivorId = actor.id;
        } else {
            Actor::Die(actor.id);
        }
    }
    EXPECT_GT(griffinCount, 0);
    EXPECT_EQ(survivorId != -1, leaveOneAlive);
    return survivorId;
}

void assertReportSourceContract(unsigned int reportEvent, int sethId) {
    const std::span<const EvtInstruction> instructions = engine->_globalEventMap.function(reportEvent);
    auto has = [&instructions](EvtOpcode opcode, EvtVariable variable, int value) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return changesVariable(instruction, opcode, variable, value);
        });
    };

    EXPECT_TRUE(has(EVENT_Compare, VAR_QBits_QuestsDone, std::to_underlying(QBIT_ERATHIA_GRIFFINS_KILLED)));
    EXPECT_TRUE(has(EVENT_Compare, VAR_QBits_QuestsDone,
                    std::to_underlying(QBIT_BRACADA_DESERT_GRIFFINS_KILLED)));
    EXPECT_TRUE(has(EVENT_Subtract, VAR_QBits_QuestsDone, std::to_underlying(QBIT_GRIFFINS_ACTIVE)));
    EXPECT_TRUE(has(EVENT_Add, VAR_FixedGold, 5000));
    EXPECT_TRUE(has(EVENT_Add, VAR_Experience, 10000));
    EXPECT_TRUE(has(EVENT_Subtract, VAR_ReputationInCurrentLocation, 10));
    EXPECT_TRUE(has(EVENT_Add, VAR_Award, std::to_underlying(AWARD_DEYJA_KILLED_ALL_GRIFFINS)));
    EXPECT_TRUE(std::ranges::any_of(instructions, [sethId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == sethId &&
               instruction.data.npc_topic_descr.index == 0 && instruction.data.npc_topic_descr.event_id == 0;
    }));
    EXPECT_TRUE(std::ranges::any_of(instructions, [sethId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCGreeting && instruction.data.npc_descr.npc_id == sethId &&
               instruction.data.npc_descr.greeting == 166;
    }));
}

struct GriffinQuestEvents {
    NPCData *seth = nullptr;
    unsigned int offer = 0;
    unsigned int report = 0;
};

GriffinQuestEvents griffinQuestEvents() {
    NPCData *seth = findSethDrakkson();
    EXPECT_NE(seth, nullptr);
    if (!seth)
        return {};

    const unsigned int offer = findNpcEvent(seth, [](const EvtInstruction &instruction) {
        return changesVariable(instruction, EVENT_Set, VAR_QBits_QuestsDone,
                               std::to_underlying(QBIT_GRIFFINS_ACTIVE)) ||
               changesVariable(instruction, EVENT_Add, VAR_QBits_QuestsDone,
                               std::to_underlying(QBIT_GRIFFINS_ACTIVE));
    });
    const int sethId = seth - pNPCStats->pNPCData.data();
    auto reportTopic = std::ranges::find_if(engine->_globalEventMap.function(offer), [sethId](const EvtInstruction &instruction) {
        if (instruction.opcode != EVENT_SetNPCTopic || instruction.data.npc_topic_descr.npc_id != sethId ||
            instruction.data.npc_topic_descr.index != 0)
            return false;
        const unsigned int candidate = instruction.data.npc_topic_descr.event_id;
        return engine->_globalEventMap.hasEvent(candidate) &&
               std::ranges::any_of(engine->_globalEventMap.function(candidate), [](const EvtInstruction &nested) {
                   return changesVariable(nested, EVENT_Subtract, VAR_QBits_QuestsDone,
                                          std::to_underlying(QBIT_GRIFFINS_ACTIVE));
               });
    });
    ASSERT_NE(reportTopic, engine->_globalEventMap.function(offer).end());
    const unsigned int report = reportTopic == engine->_globalEventMap.function(offer).end()
                                    ? 0u : reportTopic->data.npc_topic_descr.event_id;
    EXPECT_NE(offer, 0u);
    EXPECT_EQ(report, griffinReportEvent);
    if (report != 0)
        assertReportSourceContract(report, sethId);
    return {seth, offer, report};
}

void acceptGriffinQuest(EngineController &game, const GriffinQuestEvents &events) {
    openSethDialogue(game, events.seth);
    ASSERT_TRUE(isNpcEventVisible(events.seth, events.offer));
    ASSERT_TRUE(selectNpcEvent(game, events.seth, events.offer));
    ASSERT_TRUE(pParty->_questBits[QBIT_GRIFFINS_ACTIVE]);
    ASSERT_TRUE(isNpcEventVisible(events.seth, events.report));
    leaveHouse(game);
}

void setCheckpointTransactionState() {
    pParty->SetGold(checkpointGold, true);
    pParty->SetBankGold(checkpointBankGold);
    currentLocationInfo().reputation = checkpointReputation;
    pParty->setActiveCharacterIndex(checkpointActiveCharacter);
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].experience = checkpointExperience[i];
}

void assertCheckpointTransactionState() {
    EXPECT_EQ(pParty->GetGold(), checkpointGold);
    EXPECT_EQ(pParty->GetBankGold(), checkpointBankGold);
    EXPECT_EQ(pParty->GetPartyReputation(), checkpointReputation);
    EXPECT_EQ(pParty->activeCharacterIndex(), checkpointActiveCharacter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_DEYJA_KILLED_ALL_GRIFFINS]);
    }
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void saveCheckpoint(EngineController &game) {
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Griffin pre-report checkpoint");
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
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
}

}  // namespace

GAME_TEST(MM7Griffins, RegionalSurvivorRefusesReward) {
    for (auto [leaveErathiaSurvivor, leaveBracadaSurvivor] :
         std::array{std::pair{true, true}, std::pair{true, false}, std::pair{false, true}}) {
        test.prepareForNextTest();
        game.startNewGame();
        pParty->_questBits.set(QBIT_DARK_PATH);
        const GriffinQuestEvents events = griffinQuestEvents();
        ASSERT_NE(events.seth, nullptr);
        acceptGriffinQuest(game, events);

        const int erathiaSurvivor = fixtureRegionalGriffins(game, MAP_ERATHIA, leaveErathiaSurvivor);
        const int bracadaSurvivor = fixtureRegionalGriffins(game, MAP_BRACADA_DESERT, leaveBracadaSurvivor);
        openSethDialogue(game, events.seth); // Leaving Bracada evaluates its authored regional predicate.

        EXPECT_EQ(pParty->_questBits[QBIT_ERATHIA_GRIFFINS_KILLED], !leaveErathiaSurvivor);
        EXPECT_EQ(pParty->_questBits[QBIT_BRACADA_DESERT_GRIFFINS_KILLED], !leaveBracadaSurvivor);
        EXPECT_EQ(erathiaSurvivor != -1, leaveErathiaSurvivor);
        EXPECT_EQ(bracadaSurvivor != -1, leaveBracadaSurvivor);

        const int goldBefore = pParty->GetGold();
        const int bankGoldBefore = pParty->GetBankGold();
        const int reputationBefore = pParty->GetPartyReputation();
        const unsigned int topicBefore = events.seth->dialogue_1_evt_id;
        const unsigned int greetingBefore = events.seth->greetingIndex;
        std::array<uint64_t, 4> experienceBefore;
        for (int i = 0; i < 4; ++i)
            experienceBefore[i] = pParty->pCharacters[i].experience;

        ASSERT_TRUE(isNpcEventVisible(events.seth, events.report));
        ASSERT_TRUE(selectNpcEvent(game, events.seth, events.report));
        EXPECT_TRUE(pParty->_questBits[QBIT_GRIFFINS_ACTIVE]);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetBankGold(), bankGoldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        EXPECT_EQ(events.seth->dialogue_1_evt_id, topicBefore);
        EXPECT_EQ(events.seth->greetingIndex, greetingBefore);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
            EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_DEYJA_KILLED_ALL_GRIFFINS]);
        }
        EXPECT_TRUE(isNpcEventVisible(events.seth, events.report));
    }
}

GAME_TEST(MM7Griffins, BothRegionsClearGrantsExactRewardOnce) {
    game.startNewGame();
    pParty->_questBits.set(QBIT_DARK_PATH);
    pParty->SetGold(1234, false);
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].experience = 2000 + i * 100;
    pParty->setActiveCharacterIndex(2);

    const GriffinQuestEvents events = griffinQuestEvents();
    ASSERT_NE(events.seth, nullptr);
    acceptGriffinQuest(game, events);
    fixtureRegionalGriffins(game, MAP_ERATHIA, false);
    fixtureRegionalGriffins(game, MAP_BRACADA_DESERT, false);
    openSethDialogue(game, events.seth); // Leaving Bracada evaluates its authored regional predicate.

    ASSERT_TRUE(pParty->_questBits[QBIT_ERATHIA_GRIFFINS_KILLED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_BRACADA_DESERT_GRIFFINS_KILLED]);
    ASSERT_TRUE(isNpcEventVisible(events.seth, events.report));
    const int goldBefore = pParty->GetGold();
    const int bankGoldBefore = pParty->GetBankGold();
    const int reputationBefore = pParty->GetPartyReputation();
    std::array<uint64_t, 4> experienceBefore;
    for (int i = 0; i < 4; ++i)
        experienceBefore[i] = pParty->pCharacters[i].experience;

    ASSERT_TRUE(selectNpcEvent(game, events.seth, events.report));
    EXPECT_FALSE(pParty->_questBits[QBIT_GRIFFINS_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ERATHIA_GRIFFINS_KILLED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_BRACADA_DESERT_GRIFFINS_KILLED]);
    EXPECT_EQ(pParty->GetGold() - goldBefore, 5000);
    EXPECT_EQ(pParty->GetBankGold(), bankGoldBefore);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -10);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 10000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_DEYJA_KILLED_ALL_GRIFFINS]);
    }
    EXPECT_EQ(events.seth->dialogue_1_evt_id, 0u);
    EXPECT_EQ(events.seth->greetingIndex, 166u);

    const int goldAfter = pParty->GetGold();
    const int reputationAfter = pParty->GetPartyReputation();
    std::array<uint64_t, 4> experienceAfter;
    for (int i = 0; i < 4; ++i)
        experienceAfter[i] = pParty->pCharacters[i].experience;
    leaveHouse(game);
    openSethDialogue(game, events.seth);
    EXPECT_FALSE(std::ranges::contains(npcEvents(events.seth), events.report));
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationAfter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_DEYJA_KILLED_ALL_GRIFFINS]);
    }
}

GAME_TEST(MM7GriffinCheckpoint, DISABLED_WritePreReport) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_DARK_PATH);

    const GriffinQuestEvents events = griffinQuestEvents();
    ASSERT_NE(events.seth, nullptr);
    acceptGriffinQuest(game, events);
    fixtureRegionalGriffins(game, MAP_ERATHIA, false);
    fixtureRegionalGriffins(game, MAP_BRACADA_DESERT, false);
    openSethDialogue(game, events.seth);
    setCheckpointTransactionState();

    ASSERT_TRUE(pParty->_questBits[QBIT_GRIFFINS_ACTIVE]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ERATHIA_GRIFFINS_KILLED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_BRACADA_DESERT_GRIFFINS_KILLED]);
    ASSERT_EQ(events.seth->dialogue_1_evt_id, events.report);
    ASSERT_TRUE(isNpcEventVisible(events.seth, events.report));
    ASSERT_NE(events.seth->greetingIndex, 166u);
    assertCheckpointTransactionState();
    ASSERT_FALSE(::testing::Test::HasFailure());
    leaveHouse(game);

    ASSERT_EQ(engine->_currentLoadedMapId, MAP_DEYJA);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    saveCheckpoint(game);
}

GAME_TEST(MM7GriffinCheckpoint, DISABLED_ReloadPreReport) {
    assertDisposableHeadlessProfile();
    Blob saved;
    reloadCheckpoint(game, saved);

    ASSERT_EQ(engine->_currentLoadedMapId, MAP_DEYJA);
    ASSERT_TRUE(pParty->_questBits[QBIT_GRIFFINS_ACTIVE]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ERATHIA_GRIFFINS_KILLED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_BRACADA_DESERT_GRIFFINS_KILLED]);
    NPCData *seth = findSethWithReportTopic();
    ASSERT_NE(seth, nullptr);
    ASSERT_EQ(seth->dialogue_1_evt_id, griffinReportEvent);
    ASSERT_NE(seth->greetingIndex, 166u);
    assertCheckpointTransactionState();

    openSethDialogue(game, seth);
    ASSERT_TRUE(isNpcEventVisible(seth, griffinReportEvent));
    ASSERT_TRUE(selectNpcEvent(game, seth, griffinReportEvent));

    EXPECT_FALSE(pParty->_questBits[QBIT_GRIFFINS_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ERATHIA_GRIFFINS_KILLED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_BRACADA_DESERT_GRIFFINS_KILLED]);
    EXPECT_EQ(pParty->GetGold(), checkpointGold + 5000);
    EXPECT_EQ(pParty->GetBankGold(), checkpointBankGold);
    EXPECT_EQ(pParty->GetPartyReputation(), checkpointReputation - 10);
    EXPECT_EQ(pParty->activeCharacterIndex(), checkpointActiveCharacter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperience[i] + 10000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_DEYJA_KILLED_ALL_GRIFFINS]);
    }
    EXPECT_EQ(seth->dialogue_1_evt_id, 0u);
    EXPECT_FALSE(std::ranges::contains(npcEvents(seth), griffinReportEvent));
    EXPECT_EQ(seth->greetingIndex, 166u);

    leaveHouse(game);
    openSethDialogue(game, seth);
    EXPECT_FALSE(std::ranges::contains(npcEvents(seth), griffinReportEvent));
    EXPECT_EQ(pParty->GetGold(), checkpointGold + 5000);
    EXPECT_EQ(pParty->GetBankGold(), checkpointBankGold);
    EXPECT_EQ(pParty->GetPartyReputation(), checkpointReputation - 10);
    EXPECT_EQ(pParty->activeCharacterIndex(), checkpointActiveCharacter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperience[i] + 10000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_DEYJA_KILLED_ALL_GRIFFINS]);
    }
    EXPECT_EQ(seth->dialogue_1_evt_id, 0u);
    EXPECT_EQ(seth->greetingIndex, 166u);
    leaveHouse(game);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
