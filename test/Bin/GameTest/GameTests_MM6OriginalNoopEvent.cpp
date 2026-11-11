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
#include "Engine/Tables/NPCTable.h"
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

EvtProgram syntheticTimerTriggerEvent() {
    std::string bytes;
    appendRecord(&bytes, 0, EVENT_OnTimer, std::string(10, '\0'));

    std::string mutation;
    mutation.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&mutation, 20);
    appendRecord(&bytes, 1, EVENT_Set, mutation);
    appendRecord(&bytes, 2, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticSetNpcTopicEvent() {
    std::string bytes;
    std::string topic;
    appendU32(&topic, 1);   // NPC.
    topic.push_back(2);     // Third MM6 script topic slot.
    appendU32(&topic, 364); // Highest accepted MM6 global event ID.
    appendRecord(&bytes, 0, EVENT_SetNPCTopic, topic);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticMoveNpcEvent() {
    std::string bytes;
    std::string move;
    appendU32(&move, 1);   // NPC.
    appendU32(&move, 486); // Valid retained MM6 house ID.
    appendRecord(&bytes, 0, EVENT_MoveNPC, move);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticCheckSkillEvent() {
    std::string bytes;
    std::string check;
    check.push_back(static_cast<char>(SKILL_PERCEPTION));
    check.push_back(static_cast<char>(MASTERY_NONE));
    appendU32(&check, 8);
    check.push_back(3);
    appendRecord(&bytes, 0, EVENT_CheckSkill, check);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 21);
    appendRecord(&bytes, 1, EVENT_Set, failed);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 22);
    appendRecord(&bytes, 3, EVENT_Set, passed);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticCanShowGoldTopicEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(VAR_FixedGold));
    appendU32(&compare, 100);
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_OnCanShowDialogItemCmp, compare);
    appendRecord(&bytes, 1, EVENT_SetCanShowDialogItem, std::string(1, '\0'));
    appendRecord(&bytes, 2, EVENT_EndCanShowDialogItem, std::string(1, '\0'));
    appendRecord(&bytes, 3, EVENT_SetCanShowDialogItem, std::string(1, '\1'));
    appendRecord(&bytes, 4, EVENT_EndCanShowDialogItem, std::string(1, '\0'));
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticAutonoteEvent() {
    std::string bytes;
    std::string autonote;
    autonote.push_back(static_cast<char>(VAR_AutoNotes));
    appendU32(&autonote, 128); // Last retained MM6 autonote row.
    appendRecord(&bytes, 0, EVENT_Set, autonote);

    std::string compare = autonote;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 23);
    appendRecord(&bytes, 2, EVENT_Set, failed);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 24);
    appendRecord(&bytes, 4, EVENT_Set, passed);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticMonthCompareEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(226)); // MM6 month-of-year variable.
    appendU32(&compare, 6);
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 25);
    appendRecord(&bytes, 1, EVENT_Set, failed);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 26);
    appendRecord(&bytes, 3, EVENT_Set, passed);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticQuestBitEvent() {
    std::string bytes;
    std::string questBit;
    questBit.push_back(static_cast<char>(VAR_QBits_QuestsDone));
    appendU32(&questBit, 511); // Upper boundary of the MM6 quest-bit state.
    appendRecord(&bytes, 0, EVENT_Set, questBit);

    std::string compare = questBit;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 27);
    appendRecord(&bytes, 2, EVENT_Set, failed);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 28);
    appendRecord(&bytes, 4, EVENT_Set, passed);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticDayCounterEvent() {
    std::string bytes;
    std::string counter;
    counter.push_back(static_cast<char>(216)); // First MM6 day-counter variable.
    appendU32(&counter, 0);
    appendRecord(&bytes, 0, EVENT_Set, counter);

    std::string compare = counter;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 29);
    appendRecord(&bytes, 2, EVENT_Set, failed);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 30);
    appendRecord(&bytes, 4, EVENT_Set, passed);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticGoldPaymentEvent() {
    std::string bytes;
    std::string reward;
    reward.push_back(static_cast<char>(VAR_FixedGold));
    appendU32(&reward, 300);
    appendRecord(&bytes, 0, EVENT_Add, reward);

    std::string payment;
    payment.push_back(static_cast<char>(VAR_FixedGold));
    appendU32(&payment, 800);
    appendRecord(&bytes, 1, EVENT_Subtract, payment);

    std::string completed;
    completed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&completed, 31);
    appendRecord(&bytes, 2, EVENT_Set, completed);
    appendRecord(&bytes, 3, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticDayOfYearCompareEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(VAR_DayOfYear));
    appendU32(&compare, 84);
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 32);
    appendRecord(&bytes, 1, EVENT_Set, failed);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 33);
    appendRecord(&bytes, 3, EVENT_Set, passed);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticHeldItemCompareEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(VAR_PlayerItemInHands));
    appendU32(&compare, 580); // Upper boundary of MM6 item-event IDs.
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 34);
    appendRecord(&bytes, 1, EVENT_Set, failed);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 35);
    appendRecord(&bytes, 3, EVENT_Set, passed);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticQuestBitClearEvent() {
    std::string bytes;
    std::string questBit;
    questBit.push_back(static_cast<char>(VAR_QBits_QuestsDone));
    appendU32(&questBit, 511);
    appendRecord(&bytes, 0, EVENT_Subtract, questBit);

    std::string compare = questBit;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string cleared;
    cleared.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&cleared, 36);
    appendRecord(&bytes, 2, EVENT_Set, cleared);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string stillSet;
    stillSet.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&stillSet, 37);
    appendRecord(&bytes, 4, EVENT_Set, stillSet);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticNpcHireEvent() {
    std::string bytes;
    std::string npc;
    npc.push_back(static_cast<char>(214)); // MM6 NPC-in-party variable.
    appendU32(&npc, 13);
    appendRecord(&bytes, 0, EVENT_Set, npc);

    std::string compare = npc;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 38);
    appendRecord(&bytes, 2, EVENT_Set, failed);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 39);
    appendRecord(&bytes, 4, EVENT_Set, passed);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticAutonoteClearEvent() {
    std::string bytes;
    std::string autonote;
    autonote.push_back(static_cast<char>(VAR_AutoNotes));
    appendU32(&autonote, 128);
    appendRecord(&bytes, 0, EVENT_Subtract, autonote);

    std::string compare = autonote;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string cleared;
    cleared.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&cleared, 40);
    appendRecord(&bytes, 2, EVENT_Set, cleared);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string stillSet;
    stillSet.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&stillSet, 41);
    appendRecord(&bytes, 4, EVENT_Set, stillSet);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticHeldItemSubtractEvent() {
    std::string bytes;
    std::string item;
    item.push_back(static_cast<char>(VAR_PlayerItemInHands));
    appendU32(&item, 580);
    appendRecord(&bytes, 0, EVENT_Subtract, item);

    std::string continued;
    continued.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&continued, 42);
    appendRecord(&bytes, 1, EVENT_Set, continued);
    appendRecord(&bytes, 2, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticCircusPrizeCompareEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(224)); // MM6 circus-prize score variable.
    appendU32(&compare, 1);
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&failed, 43);
    appendRecord(&bytes, 1, EVENT_Set, failed);
    appendRecord(&bytes, 2, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213)); // MM6 character event bit.
    appendU32(&passed, 44);
    appendRecord(&bytes, 3, EVENT_Set, passed);
    appendRecord(&bytes, 4, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticCharacterBitCompareEvent() {
    std::string bytes;
    std::string characterBit;
    characterBit.push_back(static_cast<char>(213)); // MM6 character event-bit variable.
    appendU32(&characterBit, 511);
    appendRecord(&bytes, 0, EVENT_Set, characterBit);

    std::string compare = characterBit;
    compare.push_back(4);
    appendRecord(&bytes, 1, EVENT_Compare, compare);

    std::string failed;
    failed.push_back(static_cast<char>(213));
    appendU32(&failed, 45);
    appendRecord(&bytes, 2, EVENT_Set, failed);
    appendRecord(&bytes, 3, EVENT_Exit);

    std::string passed;
    passed.push_back(static_cast<char>(213));
    appendU32(&passed, 46);
    appendRecord(&bytes, 4, EVENT_Set, passed);
    appendRecord(&bytes, 5, EVENT_Exit);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticCanShowHeldItemTopicEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(VAR_PlayerItemInHands));
    appendU32(&compare, 580);
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_OnCanShowDialogItemCmp, compare);
    appendRecord(&bytes, 1, EVENT_SetCanShowDialogItem, std::string(1, '\0'));
    appendRecord(&bytes, 2, EVENT_EndCanShowDialogItem, std::string(1, '\0'));
    appendRecord(&bytes, 3, EVENT_SetCanShowDialogItem, std::string(1, '\1'));
    appendRecord(&bytes, 4, EVENT_EndCanShowDialogItem, std::string(1, '\0'));
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

EvtProgram syntheticCanShowDayOfYearTopicEvent() {
    std::string bytes;
    std::string compare;
    compare.push_back(static_cast<char>(VAR_DayOfYear));
    appendU32(&compare, 84);
    compare.push_back(3);
    appendRecord(&bytes, 0, EVENT_OnCanShowDialogItemCmp, compare);
    appendRecord(&bytes, 1, EVENT_SetCanShowDialogItem, std::string(1, '\0'));
    appendRecord(&bytes, 2, EVENT_EndCanShowDialogItem, std::string(1, '\0'));
    appendRecord(&bytes, 3, EVENT_SetCanShowDialogItem, std::string(1, '\1'));
    appendRecord(&bytes, 4, EVENT_EndCanShowDialogItem, std::string(1, '\0'));
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
}

struct ScopedLocalEventMap {
    EvtProgram events = std::move(engine->_localEventMap);

    ~ScopedLocalEventMap() {
        engine->_localEventMap = std::move(events);
    }
};

struct ScopedGlobalEventMap {
    EvtProgram events = std::move(engine->_globalEventMap);

    ~ScopedGlobalEventMap() {
        engine->_globalEventMap = std::move(events);
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

struct ScopedNpcStats {
    NPCStats stats = *pNPCStats;

    ~ScopedNpcStats() {
        *pNPCStats = std::move(stats);
    }
};

struct ScopedParty {
    Party party = *pParty;

    ~ScopedParty() {
        *pParty = std::move(party);
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

GAME_TEST(MM6TimerTrigger, StopsDirectEventExecution) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restore;
    engine->_localEventMap = syntheticTimerTriggerEvent();
    pParty->pCharacters[0]._characterEventBits.reset(20);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(20));
}

GAME_TEST(MM6SetNpcTopic, UpdatesTheMM6TopicSlot) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedNpcStats restoreNpcs;
    engine->_localEventMap = syntheticSetNpcTopicEvent();
    pNPCStats->pNPCData[1].dialogue_1_evt_id = 101;
    pNPCStats->pNPCData[1].dialogue_2_evt_id = 102;
    pNPCStats->pNPCData[1].dialogue_3_evt_id = 0;

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pNPCStats->pNPCData[1].dialogue_1_evt_id, 101);
    EXPECT_EQ(pNPCStats->pNPCData[1].dialogue_2_evt_id, 102);
    EXPECT_EQ(pNPCStats->pNPCData[1].dialogue_3_evt_id, 364);
}

GAME_TEST(MM6MoveNpc, UpdatesTheMM6NpcLocation) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedNpcStats restoreNpcs;
    engine->_localEventMap = syntheticMoveNpcEvent();
    pNPCStats->pNPCData[1].house = HOUSE_INVALID;

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pNPCStats->pNPCData[1].house, static_cast<HouseId>(486));
}

GAME_TEST(MM6CheckSkill, UsesTheMM6ZeroMasteryThresholdRule) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedQuestionEvent restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticCheckSkillEvent();
    pParty->setActiveCharacterIndex(0);
    pParty->pCharacters[0].pActiveSkills[SKILL_PERCEPTION] = CombinedSkillValue::novice(8);
    pParty->pCharacters[0]._characterEventBits.reset(21);
    pParty->pCharacters[0]._characterEventBits.reset(22);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(21));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(22));
}

GAME_TEST(MM6TopicAdmission, UsesMM6GoldComparisonToSelectTheTopic) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedGlobalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_globalEventMap = syntheticCanShowGoldTopicEvent();
    pParty->SetGold(100, true);

    EvtInterpreter interpreter;
    interpreter.prepare(EvtContext(EVT_SOURCE_GLOBAL, 1));
    EXPECT_TRUE(interpreter.canShowTopic());

    pParty->SetGold(99, true);
    EvtInterpreter denied;
    denied.prepare(EvtContext(EVT_SOURCE_GLOBAL, 1));
    EXPECT_FALSE(denied.canShowTopic());
}

GAME_TEST(MM6AutonoteEvent, UsesTheMM6AutonoteVariableAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticAutonoteEvent();
    pParty->_autonoteBits.reset(128);
    pParty->pCharacters[0]._characterEventBits.reset(23);
    pParty->pCharacters[0]._characterEventBits.reset(24);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pParty->_autonoteBits.test(128));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(23));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(24));
}

GAME_TEST(MM6MonthCompare, UsesTheMM6MonthVariableAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticMonthCompareEvent();
    pParty->uCurrentMonth = 6;
    pParty->pCharacters[0]._characterEventBits.reset(25);
    pParty->pCharacters[0]._characterEventBits.reset(26);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(25));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(26));

    pParty->uCurrentMonth = 5;
    pParty->pCharacters[0]._characterEventBits.reset(25);
    pParty->pCharacters[0]._characterEventBits.reset(26);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(25));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(26));
}

GAME_TEST(MM6QuestBitEvent, UsesTheMM6QuestBitStateAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticQuestBitEvent();
    pParty->_questBits.reset(static_cast<QuestBit>(511));
    pParty->_questBits.reset(static_cast<QuestBit>(510));
    pParty->pCharacters[0]._characterEventBits.reset(27);
    pParty->pCharacters[0]._characterEventBits.reset(28);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pParty->_questBits.test(static_cast<QuestBit>(511)));
    EXPECT_FALSE(pParty->_questBits.test(static_cast<QuestBit>(510)));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(27));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(28));
}

GAME_TEST(MM6DayCounterEvent, UsesTheMM6DayCounterStateAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticDayCounterEvent();
    pParty->GetPlayingTime() = Time::fromDays(42);
    pParty->PartyTimes.CounterEventValues[0] = Time();
    pParty->pCharacters[0]._characterEventBits.reset(29);
    pParty->pCharacters[0]._characterEventBits.reset(30);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pParty->PartyTimes.CounterEventValues[0], Time::fromDays(42));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(29));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(30));
}

GAME_TEST(MM6GoldPaymentEvent, StopsOnInsufficientMM6Gold) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticGoldPaymentEvent();
    pParty->pCharacters[0]._characterEventBits.reset(31);

    pParty->SetGold(500, true);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pParty->GetGold(), 0);
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(31));

    pParty->SetGold(499, true);
    pParty->pCharacters[0]._characterEventBits.reset(31);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pParty->GetGold(), 799);
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(31));
}

GAME_TEST(MM6DayOfYearCompare, UsesTheMM6OneBasedCalendar) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticDayOfYearCompareEvent();
    pParty->pCharacters[0]._characterEventBits.reset(32);
    pParty->pCharacters[0]._characterEventBits.reset(33);

    pParty->GetPlayingTime() = Time::fromDays(83);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(32));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(33));

    pParty->GetPlayingTime() = Time::fromDays(82);
    pParty->pCharacters[0]._characterEventBits.reset(32);
    pParty->pCharacters[0]._characterEventBits.reset(33);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(32));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(33));
}

GAME_TEST(MM6HeldItemCompare, UsesTheMM6HeldItemAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticHeldItemCompareEvent();
    pParty->pCharacters[0]._characterEventBits.reset(34);
    pParty->pCharacters[0]._characterEventBits.reset(35);

    pParty->pPickedItem.itemId = static_cast<ItemId>(580);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(34));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(35));

    pParty->pPickedItem.itemId = static_cast<ItemId>(579);
    pParty->pCharacters[0]._characterEventBits.reset(34);
    pParty->pCharacters[0]._characterEventBits.reset(35);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(34));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(35));
}

GAME_TEST(MM6QuestBitClear, ClearsTheMM6QuestBitAndFallsThrough) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticQuestBitClearEvent();
    pParty->_questBits.set(static_cast<QuestBit>(511));
    pParty->pCharacters[0]._characterEventBits.reset(36);
    pParty->pCharacters[0]._characterEventBits.reset(37);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->_questBits.test(static_cast<QuestBit>(511)));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(36));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(37));
}

GAME_TEST(MM6NpcHireEvent, UsesTheMM6NpcInPartyStateAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedNpcStats restoreNpcs;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticNpcHireEvent();
    pNPCStats->pNPCData[13].flags &= ~NPC_HIRED;
    pParty->pCharacters[0]._characterEventBits.reset(38);
    pParty->pCharacters[0]._characterEventBits.reset(39);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pNPCStats->pNPCData[13].Hired());
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(38));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(39));
}

GAME_TEST(MM6AutonoteClear, ClearsTheMM6AutonoteAndFallsThrough) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticAutonoteClearEvent();
    pParty->_autonoteBits.set(128);
    pParty->pCharacters[0]._characterEventBits.reset(40);
    pParty->pCharacters[0]._characterEventBits.reset(41);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->_autonoteBits.test(128));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(40));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(41));
}

GAME_TEST(MM6HeldItemSubtract, ConsumesOnlyTheMatchingMM6HeldItem) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticHeldItemSubtractEvent();
    for (Character &character : pParty->pCharacters)
        character.inventory.clear();
    pParty->pCharacters[0]._characterEventBits.reset(42);

    pParty->pPickedItem.itemId = static_cast<ItemId>(580);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(42));

    pParty->pPickedItem.itemId = static_cast<ItemId>(579);
    pParty->pCharacters[0]._characterEventBits.reset(42);
    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_EQ(pParty->pPickedItem.itemId, static_cast<ItemId>(579));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(42));
}

GAME_TEST(MM6CircusPrizeCompare, UsesTheMM6PartyInventoryScoreAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticCircusPrizeCompareEvent();
    for (Character &character : pParty->pCharacters)
        character.inventory.clear();
    Item prize;
    prize.itemId = static_cast<ItemId>(470);
    ASSERT_TRUE(pParty->pCharacters[0].inventory.equip(ITEM_SLOT_RING1, prize));
    pParty->pCharacters[0]._characterEventBits.reset(43);
    pParty->pCharacters[0]._characterEventBits.reset(44);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(43));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(44));
}

GAME_TEST(MM6CharacterBitCompare, UsesTheMM6CharacterBitStateAndBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedLocalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_localEventMap = syntheticCharacterBitCompareEvent();
    pParty->setActiveCharacterIndex(0);
    pParty->pCharacters[0]._characterEventBits.reset(511);
    pParty->pCharacters[0]._characterEventBits.reset(45);
    pParty->pCharacters[0]._characterEventBits.reset(46);

    runEvent(EvtContext(EVT_SOURCE_MAP, 1));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(511));
    EXPECT_FALSE(pParty->pCharacters[0]._characterEventBits.test(45));
    EXPECT_TRUE(pParty->pCharacters[0]._characterEventBits.test(46));
}

GAME_TEST(MM6HeldItemTopicAdmission, UsesTheMM6HeldItemTopicBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedGlobalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_globalEventMap = syntheticCanShowHeldItemTopicEvent();

    pParty->pPickedItem.itemId = static_cast<ItemId>(580);
    EvtInterpreter accepted;
    accepted.prepare(EvtContext(EVT_SOURCE_GLOBAL, 1));
    EXPECT_TRUE(accepted.canShowTopic());

    pParty->pPickedItem.itemId = static_cast<ItemId>(579);
    EvtInterpreter refused;
    refused.prepare(EvtContext(EVT_SOURCE_GLOBAL, 1));
    EXPECT_FALSE(refused.canShowTopic());
}

GAME_TEST(MM6DayOfYearTopicAdmission, UsesTheMM6CalendarTopicBranch) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedGlobalEventMap restoreEvents;
    ScopedParty restoreParty;
    engine->_globalEventMap = syntheticCanShowDayOfYearTopicEvent();

    pParty->GetPlayingTime() = Time::fromDays(83);
    EvtInterpreter accepted;
    accepted.prepare(EvtContext(EVT_SOURCE_GLOBAL, 1));
    EXPECT_TRUE(accepted.canShowTopic());

    pParty->GetPlayingTime() = Time::fromDays(82);
    EvtInterpreter refused;
    refused.prepare(EvtContext(EVT_SOURCE_GLOBAL, 1));
    EXPECT_FALSE(refused.canShowTopic());
}
