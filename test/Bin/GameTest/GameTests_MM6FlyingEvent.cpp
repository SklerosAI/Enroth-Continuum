#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Party.h"
#include "Engine/Spells/SpellBuff.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedFlyingState {
    unsigned int previousFlag = pParty->bFlying;
    SpellBuff previousBuff = pParty->pPartyBuffs[PARTY_BUFF_FLY];

    ~ScopedFlyingState() {
        pParty->bFlying = previousFlag;
        pParty->pPartyBuffs[PARTY_BUFF_FLY] = previousBuff;
    }
};

EvtInstruction syntheticFlyingCompare() {
    // Synthetic MM6 Compare: event 1, raw variable 222, operand 0, jump 4.
    const std::string bytes("\x0a\x01\x00\x00\x0e\xde\x00\x00\x00\x00\x04", 11);
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6).instruction(1, 0);
}

} // namespace

GAME_TEST(MM6FlyingEvent, InterpreterRequiresFlightAndBuff) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedFlyingState restore;
    EvtInstruction instruction = syntheticFlyingCompare();
    EvtInterpreter interpreter;

    pParty->bFlying = false;
    pParty->pPartyBuffs[PARTY_BUFF_FLY].GetExpireTime() = Time();
    EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_NEXT);

    pParty->bFlying = true;
    EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_NEXT);

    pParty->pPartyBuffs[PARTY_BUFF_FLY].GetExpireTime() = Time::fromDays(1);
    EvtFlow matched = interpreter.executeInstruction(instruction);
    EXPECT_EQ(matched.type, EVT_FLOW_JUMP);
    EXPECT_EQ(matched.target, 4);

    pParty->bFlying = false;
    EXPECT_EQ(interpreter.executeInstruction(instruction).type, EVT_FLOW_NEXT);
}
