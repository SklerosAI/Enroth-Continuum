#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedPartyAndNpcs {
    Party party = *pParty;
    NPCStats npcs = *pNPCStats;

    ~ScopedPartyAndNpcs() {
        *pParty = party;
        *pNPCStats = npcs;
    }
};

std::string variableWrite(char step, char opcode, unsigned char variable, char value) {
    return std::string("\x09\x01\x00", 3) + step + opcode + static_cast<char>(variable) + value + std::string(3, '\0');
}

std::string variableCompare(char step, unsigned char variable, char value, char target) {
    return std::string("\x0a\x01\x00", 3) + step + '\x0e' + static_cast<char>(variable) + value + std::string(3, '\0') +
           target;
}

std::string forPartyMember(char step, char who) {
    return std::string("\x05\x01\x00", 3) + step + '\x23' + who;
}

EvtProgram syntheticRewardEvent() {
    // Add 10 to fire resistance, Set magic resistance bonus to 20, Add 20 skill points, hire NPC 13 and check that
    // it's hired, then make the second character dead. Shipped MM6 scripts use each of these operands.
    return EvtProgram::load(Blob::fromString(variableWrite(0, '\x10', 46, 10) +
                                             variableWrite(1, '\x12', 55, 20) +
                                             variableWrite(2, '\x10', 225, 20) +
                                             variableWrite(3, '\x12', 214, 13) +
                                             variableCompare(4, 214, 13, 7) +
                                             forPartyMember(5, 1) +
                                             variableWrite(6, '\x12', 101, 0)),
                            GameTitle::MM6);
}

} // namespace

GAME_TEST(MM6EventVariables, InterpreterUsesMM6ResistanceSkillPointHireAndConditionStorage) {
    // MM6 scripts that touch resistances, skill points, hired NPCs or Dead used to throw before mutating anything.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedPartyAndNpcs restore;
    EvtProgram program = syntheticRewardEvent();
    EvtInterpreter interpreter;

    for (Character &character : pParty->pCharacters) {
        character.sResFireBase = 5;
        character.sResMagicBonus = 0;
        character.uSkillPoints = 3;
        character.conditions.resetAll();
    }
    pNPCStats->pNPCData[13].flags &= ~NPC_HIRED;
    pParty->GetPlayingTime() = Time::fromHours(9); // A condition stamped at time zero reads as absent.

    for (int step = 0; step < 4; ++step)
        EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, step)).type, EVT_FLOW_NEXT);
    for (const Character &character : pParty->pCharacters) {
        EXPECT_EQ(character.sResFireBase, 15);
        EXPECT_EQ(character.sResAirBase, 0);
        EXPECT_EQ(character.sResMagicBonus, 20);
        EXPECT_EQ(character.sResPhysicalBonus, 0);
        EXPECT_EQ(character.uSkillPoints, 23);
    }
    EXPECT_TRUE(pNPCStats->pNPCData[13].Hired());

    EvtFlow hired = interpreter.executeInstruction(program.instruction(1, 4));
    EXPECT_EQ(hired.type, EVT_FLOW_JUMP);
    EXPECT_EQ(hired.target, 7);

    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 5)).type, EVT_FLOW_NEXT);
    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 6)).type, EVT_FLOW_NEXT);
    EXPECT_FALSE(pParty->pCharacters[0].conditions.has(CONDITION_DEAD));
    EXPECT_TRUE(pParty->pCharacters[1].conditions.has(CONDITION_DEAD));
    EXPECT_FALSE(pParty->pCharacters[2].conditions.has(CONDITION_DEAD));
}
