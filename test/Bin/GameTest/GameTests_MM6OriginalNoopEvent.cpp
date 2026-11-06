#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Party.h"

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
