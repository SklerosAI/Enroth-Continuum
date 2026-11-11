#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Party.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIStatusBar.h"

#include "Utility/Memory/Blob.h"

namespace {

EvtInstruction parsedMM6Instruction(EvtOpcode opcode, std::string payload) {
    std::string record;
    record.push_back(static_cast<char>(4 + payload.size()));
    record.push_back(1); // Event ID, little endian.
    record.push_back(0);
    record.push_back(0); // Step.
    record.push_back(static_cast<char>(opcode));
    record += payload;
    return EvtProgram::load(Blob::fromString(record), GameTitle::MM6).instruction(1, 0);
}

void appendU32(std::string *bytes, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes->push_back(static_cast<char>(value >> (8 * i)));
}

void appendRecord(std::string *bytes, int step, EvtOpcode opcode, std::string payload = {}) {
    bytes->push_back(static_cast<char>(4 + payload.size()));
    bytes->push_back(1); // Event ID.
    bytes->push_back(0);
    bytes->push_back(static_cast<char>(step));
    bytes->push_back(static_cast<char>(opcode));
    *bytes += payload;
}

EvtProgram syntheticQuestionEvent() {
    std::string bytes;
    std::string question;
    appendU32(&question, 0); // Question.
    appendU32(&question, 1); // First accepted answer.
    appendU32(&question, 2); // Second accepted answer.
    question.push_back(3);   // Correct-answer continuation.
    appendRecord(&bytes, 0, EVENT_InputString, question);

    // Wrong answers take step 1 and stop before the correct-answer branch.
    std::string incorrect;
    incorrect.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&incorrect, 10);
    appendRecord(&bytes, 1, EVENT_Set, incorrect);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string correct;
    correct.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&correct, 11);
    appendRecord(&bytes, 3, EVENT_Set, correct);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticPressAnyKeyEvent() {
    std::string bytes;
    appendRecord(&bytes, 0, EVENT_PressAnyKey, std::string(1, '\0'));

    std::string continuation;
    continuation.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&continuation, 12);
    appendRecord(&bytes, 1, EVENT_Set, continuation);
    appendRecord(&bytes, 2, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticMessageThenPressAnyKeyEvent() {
    std::string bytes;
    std::string message;
    appendU32(&message, 0);
    appendRecord(&bytes, 0, EVENT_ShowMessage, message);
    appendRecord(&bytes, 1, EVENT_PressAnyKey, std::string(1, '\0'));

    std::string continuation;
    continuation.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&continuation, 13);
    appendRecord(&bytes, 2, EVENT_Set, continuation);
    appendRecord(&bytes, 3, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticSpeakNpcEvent() {
    std::string bytes;
    std::string npc;
    appendU32(&npc, 42);
    appendRecord(&bytes, 0, EVENT_SpeakNPC, npc);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticStatusTextEvent(int stringId) {
    std::string bytes;
    std::string text;
    appendU32(&text, stringId);
    appendRecord(&bytes, 0, EVENT_StatusText, text);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticJumpEvent() {
    std::string bytes;
    appendRecord(&bytes, 0, EVENT_Jmp, std::string(1, '\3'));

    std::string skipped;
    skipped.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&skipped, 14);
    appendRecord(&bytes, 1, EVENT_Set, skipped);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string target;
    target.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&target, 15);
    appendRecord(&bytes, 3, EVENT_Set, target);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticMapReloadTriggerEvent() {
    std::string bytes;
    appendRecord(&bytes, 0, EVENT_OnMapReload, std::string(1, '\0'));

    std::string mutation;
    mutation.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&mutation, 16);
    appendRecord(&bytes, 1, EVENT_Set, mutation);
    appendRecord(&bytes, 2, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticRandomGotoEvent() {
    std::string bytes;
    appendRecord(&bytes, 0, EVENT_RandomGoTo, std::string("\x03\x05\0\0\0\0", 6));

    std::string fallthrough;
    fallthrough.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&fallthrough, 17);
    appendRecord(&bytes, 1, EVENT_Set, fallthrough);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string firstTarget;
    firstTarget.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&firstTarget, 18);
    appendRecord(&bytes, 3, EVENT_Set, firstTarget);
    appendRecord(&bytes, 4, EVENT_Exit);

    std::string secondTarget;
    secondTarget.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&secondTarget, 19);
    appendRecord(&bytes, 5, EVENT_Set, secondTarget);
    appendRecord(&bytes, 6, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

struct ScopedLocalEventMap {
    EvtProgram events = std::move(engine->_localEventMap);

    ~ScopedLocalEventMap() {
        engine->_localEventMap = std::move(events);
    }
};

struct ScopedDeferredNpcDialogue {
    int npcId = bDialogueUI_InitializeActor_NPC_ID;

    ~ScopedDeferredNpcDialogue() {
        bDialogueUI_InitializeActor_NPC_ID = npcId;
    }
};

struct ScopedLevelStrings {
    std::vector<std::string> strings = std::move(engine->_levelStrings);

    ~ScopedLevelStrings() {
        engine->_levelStrings = std::move(strings);
    }
};

struct ScopedQuestionEvent {
    EvtProgram events = std::move(engine->_localEventMap);
    std::vector<std::string> strings = std::move(engine->_levelStrings);
    std::array<IndexedBitset<1, 512>, 4> characterBits;
    int activeCharacter = pParty->hasActiveCharacter() ? pParty->activeCharacterIndex() : -1;
    std::string branchlessText = std::move(branchless_dialogue_str);
    std::string npcText = std::move(current_npc_text);

    ScopedQuestionEvent() {
        for (int i = 0; i < 4; ++i)
            characterBits[i] = pParty->pCharacters[i]._characterEventBits;
        engine->_localEventMap = syntheticQuestionEvent();
        engine->_levelStrings = {"What is the password?", "oracle", "seer"};
        pParty->setActiveCharacterIndex(0);
        branchless_dialogue_str.clear();
        current_npc_text.clear();
    }

    ~ScopedQuestionEvent() {
        engine->_localEventMap = std::move(events);
        engine->_levelStrings = std::move(strings);
        pParty->setActiveCharacterIndex(activeCharacter);
        branchless_dialogue_str = std::move(branchlessText);
        current_npc_text = std::move(npcText);
        for (int i = 0; i < 4; ++i)
            pParty->pCharacters[i]._characterEventBits = characterBits[i];
    }
};

void enterQuestionResponse(EngineController &game, std::span<const PlatformKey> response) {
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    ASSERT_NE(pGUIWindow_BranchlessDialogue, nullptr);
    for (PlatformKey key : response) {
        game.pressAndReleaseKey(key);
        game.tick();
    }
    game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
    game.tick(2);
    EXPECT_EQ(pGUIWindow_BranchlessDialogue, nullptr);
}

} // namespace

GAME_TEST(MM6OriginalNoopEvent, AdvancesWithoutEffect) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    EvtInterpreter interpreter;
    const int previousDeaths = pParty->uNumDeaths;

    for (auto [opcode, payload] : {
             std::pair{EVENT_MM6_Opcode20, std::string("\x47\x00\x00\x00\x01", 5)},
             std::pair{EVENT_MM6_Opcode27, std::string("\x01\x01", 2)},
             std::pair{EVENT_MM6_Opcode28, std::string("\x01", 1)}}) {
        EvtInstruction instruction = parsedMM6Instruction(opcode, payload);
        EXPECT_EQ(instruction.data.mm6_unknown_descr.length, payload.size());
        EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_NEXT);
        EXPECT_EQ(pParty->uNumDeaths, previousDeaths);

        instruction.gameTitle = GameTitle::MM7;
        EXPECT_THROW((void) interpreter.executeInstruction(instruction), std::runtime_error);
    }
}

GAME_TEST(MM6QuestionInput, ResumesCorrectAndIncorrectContinuations) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    for (Character &character : pParty->pCharacters) {
        character._characterEventBits.reset(10);
        character._characterEventBits.reset(11);
    }

    // Closing a Question must abandon the paused event, rather than choosing either branch.
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    ASSERT_NE(pGUIWindow_BranchlessDialogue, nullptr);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    EXPECT_EQ(pGUIWindow_BranchlessDialogue, nullptr);
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(10));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(11));

    const std::array wrong = {PlatformKey::KEY_N, PlatformKey::KEY_O};
    enterQuestionResponse(game, wrong);
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(10));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(11));

    for (Character &character : pParty->pCharacters)
        character._characterEventBits.reset(10);
    const std::array correct = {PlatformKey::KEY_S, PlatformKey::KEY_E, PlatformKey::KEY_E, PlatformKey::KEY_R};
    enterQuestionResponse(game, correct);
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(10));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(11));
}

GAME_TEST(MM6PressAnyKey, ResumesContinuationAfterInput) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    engine->_localEventMap = syntheticPressAnyKeyEvent();
    pParty->pCharacters[0]._characterEventBits.reset(12);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    ASSERT_NE(pGUIWindow_BranchlessDialogue, nullptr);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(2);

    EXPECT_EQ(pGUIWindow_BranchlessDialogue, nullptr);
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(12));
}

GAME_TEST(MM6MessageDialogue, BindsMapStringBeforePressAnyKeyContinuation) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    engine->_localEventMap = syntheticMessageThenPressAnyKeyEvent();
    engine->_levelStrings = {"Continue through this synthetic prompt."};
    pParty->pCharacters[0]._characterEventBits.reset(13);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(branchless_dialogue_str, "Continue through this synthetic prompt.");
    ASSERT_NE(pGUIWindow_BranchlessDialogue, nullptr);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(2);

    EXPECT_EQ(pGUIWindow_BranchlessDialogue, nullptr);
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(13));
}

GAME_TEST(MM6SpeakNpc, DefersDialogueDuringMapLoad) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedDeferredNpcDialogue restoreNpc;
    engine->_localEventMap = syntheticSpeakNpcEvent();
    bDialogueUI_InitializeActor_NPC_ID = 0;

    EvtInterpreter interpreter;
    interpreter.prepare(EvtContext(EVT_SOURCE_MAP, 1, Pid(), /* duringMapLoad = */ true));
    EXPECT_EQ(interpreter.execute(0), EVT_TRANSITION_NONE);
    EXPECT_EQ(bDialogueUI_InitializeActor_NPC_ID, 42);
}

GAME_TEST(MM6StatusText, UsesMapStringAndBoundsChecksItsId) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(engine->_statusBar, nullptr);
    ScopedLocalEventMap restoreEvents;
    ScopedLevelStrings restoreStrings;
    engine->_levelStrings = {"Synthetic MM6 status text"};
    engine->_statusBar->clearAll();

    EvtInterpreter interpreter;
    engine->_localEventMap = syntheticStatusTextEvent(0);
    interpreter.prepare(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(interpreter.execute(0), EVT_TRANSITION_NONE);
    EXPECT_EQ(engine->_statusBar->get(), "Synthetic MM6 status text");

    engine->_statusBar->clearAll();
    engine->_localEventMap = syntheticStatusTextEvent(1);
    interpreter.prepare(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(interpreter.execute(0), EVT_TRANSITION_NONE);
    EXPECT_TRUE(engine->_statusBar->get().empty());
    engine->_statusBar->clearAll();
}

GAME_TEST(MM6EventJump, SkipsFallthroughAndRunsItsTarget) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    engine->_localEventMap = syntheticJumpEvent();
    for (Character &character : pParty->pCharacters) {
        character._characterEventBits.reset(14);
        character._characterEventBits.reset(15);
    }

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(14));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(15));
}

GAME_TEST(MM6MapReloadTrigger, StopsDirectEventExecution) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    engine->_localEventMap = syntheticMapReloadTriggerEvent();
    pParty->pCharacters[0]._characterEventBits.reset(16);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(16));
}

GAME_TEST(MM6RandomGoto, RunsOneDeclaredTargetWithoutFallthrough) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    engine->_localEventMap = syntheticRandomGotoEvent();
    for (Character &character : pParty->pCharacters) {
        character._characterEventBits.reset(17);
        character._characterEventBits.reset(18);
        character._characterEventBits.reset(19);
    }

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(17));
    EXPECT_NE(pParty->pCharacters[0]._characterEventBits.test(18),
              pParty->pCharacters[0]._characterEventBits.test(19));
}
