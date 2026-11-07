#include <array>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Party.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedPartySkills {
    std::array<CombinedSkillValue, 4> perception;
    std::array<CombinedSkillValue, 4> blaster;

    ScopedPartySkills() {
        for (int i = 0; i < 4; ++i) {
            perception[i] = pParty->pCharacters[i].getSkillValue(SKILL_PERCEPTION);
            blaster[i] = pParty->pCharacters[i].getSkillValue(SKILL_BLASTER);
        }
    }

    ~ScopedPartySkills() {
        for (int i = 0; i < 4; ++i) {
            pParty->pCharacters[i].setSkillValue(SKILL_PERCEPTION, perception[i]);
            pParty->pCharacters[i].setSkillValue(SKILL_BLASTER, blaster[i]);
        }
    }
};

EvtProgram syntheticSkillEvent() {
    // MM6 Compare Perception (raw ID 82) at level four, then Set Blaster
    // (raw ID 63) to Novice one. Both operands occur in retained active EVTs.
    const std::string compare("\x0a\x01\x00\x00\x0e\x52\x04\x00\x00\x00\x02", 11);
    const std::string set("\x09\x01\x00\x01\x12\x3f\x01\x00\x00\x00", 10);
    return EvtProgram::load(Blob::fromString(compare + set), GameTitle::MM6);
}

}  // namespace

GAME_TEST(MM6SkillEvent, InterpreterUsesMM6SkillSlots) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedPartySkills restore;
    EvtProgram program = syntheticSkillEvent();
    EvtInterpreter interpreter;

    for (Character &character : pParty->pCharacters) {
        character.setSkillValue(SKILL_PERCEPTION, CombinedSkillValue::novice(3));
        character.setSkillValue(SKILL_BLASTER, CombinedSkillValue::none());
    }
    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 0)).type, EVT_FLOW_NEXT);
    pParty->pCharacters[2].setSkillValue(SKILL_PERCEPTION, CombinedSkillValue::novice(4));
    EvtFlow matched = interpreter.executeInstruction(program.instruction(1, 0));
    EXPECT_EQ(matched.type, EVT_FLOW_JUMP);
    EXPECT_EQ(matched.target, 2);

    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 1)).type, EVT_FLOW_NEXT);
    for (const Character &character : pParty->pCharacters)
        EXPECT_EQ(character.getSkillValue(SKILL_BLASTER), CombinedSkillValue::novice());
}
