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

#include "GUI/GUIWindow.h"

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

struct ScopedQuestionEvent {
    EvtProgram events = std::move(engine->_localEventMap);
    std::vector<std::string> strings = std::move(engine->_levelStrings);
    std::array<IndexedBitset<1, 512>, 4> characterBits;
    int activeCharacter = pParty->hasActiveCharacter() ? pParty->activeCharacterIndex() : -1;

    ScopedQuestionEvent() {
        for (int i = 0; i < 4; ++i)
            characterBits[i] = pParty->pCharacters[i]._characterEventBits;
        engine->_localEventMap = syntheticQuestionEvent();
        engine->_levelStrings = {"What is the password?", "oracle", "seer"};
        pParty->setActiveCharacterIndex(0);
    }

    ~ScopedQuestionEvent() {
        engine->_localEventMap = std::move(events);
        engine->_levelStrings = std::move(strings);
        pParty->setActiveCharacterIndex(activeCharacter);
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
