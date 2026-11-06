#include <stdexcept>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Party.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedPartyMonth {
    int previous = pParty->uCurrentMonth;

    ~ScopedPartyMonth() { pParty->uCurrentMonth = previous; }
};

EvtInstruction syntheticMonthCompare() {
    // Synthetic MM6 Compare: event 1, raw variable 226, month 6, jump 3.
    const std::string bytes("\x0a\x01\x00\x00\x0e\xe2\x06\x00\x00\x00\x03", 11);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6).instruction(1, 0);
}

} // namespace

GAME_TEST(MM6MonthEvent, InterpreterBranchesOnPartyCalendar) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedPartyMonth restore;
    EvtInstruction instruction = syntheticMonthCompare();
    EvtInterpreter interpreter;

    pParty->uCurrentMonth = 5;
    EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_NEXT);

    pParty->uCurrentMonth = 6;
    EvtFlow matched = interpreter.executeInstruction(instruction);
    EXPECT_EQ(matched.type, EVT_FLOW_JUMP);
    EXPECT_EQ(matched.target, 3);

    pParty->uCurrentMonth = 7;
    EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_NEXT);

    instruction.data.variable_descr.value = 0;
    pParty->uCurrentMonth = 0;
    EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_JUMP);
    instruction.data.variable_descr.value = 12;
    EXPECT_THROW((void) interpreter.executeInstruction(instruction), std::runtime_error);
}
