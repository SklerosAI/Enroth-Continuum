#include <array>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Party.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedCharacterEventBits {
    std::array<IndexedBitset<1, 512>, 4> previous;

    ScopedCharacterEventBits() {
        for (int i = 0; i < 4; ++i)
            previous[i] = pParty->pCharacters[i]._characterEventBits;
    }

    ~ScopedCharacterEventBits() {
        for (int i = 0; i < 4; ++i)
            pParty->pCharacters[i]._characterEventBits = previous[i];
    }
};

EvtProgram syntheticCharacterBitEvent() {
    // Synthetic MM6 Compare bit 7 -> step 2, then Set bit 7 at step 1.
    const std::string compare("\x0a\x01\x00\x00\x0e\xd5\x07\x00\x00\x00\x02", 11);
    const std::string set("\x09\x01\x00\x01\x12\xd5\x07\x00\x00\x00", 10);
    return EvtProgram::load(Blob::fromString(compare + set), GameTitle::MM6);
}

} // namespace

GAME_TEST(MM6CharacterBitEvent, InterpreterSetsSelectedPartyBits) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedCharacterEventBits restore;
    EvtProgram program = syntheticCharacterBitEvent();
    const EvtInstruction &compare = program.instruction(1, 0);
    const EvtInstruction &set = program.instruction(1, 1);
    EvtInterpreter interpreter;

    for (Character &character : pParty->pCharacters)
        character._characterEventBits.reset(8);
    EXPECT_EQ(interpreter.executeInstruction(compare).type, EVT_FLOW_NEXT);
    EXPECT_EQ(interpreter.executeInstruction(set).type, EVT_FLOW_NEXT);
    for (Character &character : pParty->pCharacters)
        EXPECT_TRUE(character._characterEventBits.test(8));
    EvtFlow matched = interpreter.executeInstruction(compare);
    EXPECT_EQ(matched.type, EVT_FLOW_JUMP);
    EXPECT_EQ(matched.target, 2);
}
