#pragma once

#include "Engine/Evt/EvtEnums.h"
#include "Engine/GameTitle.h"
#include "Engine/Objects/CharacterEnums.h"

class Character;

namespace mm6 {

// EVT IDs 56..86 address the 31 MM6 skill slots. The final source slot is
// Learning, whose shared runtime enum moved after MM7's extra skills.
bool isSkillVariable(int rawId);
Skill checkedEventSkill(EvtOpcode opcode, int rawId, int value, GameTitle title);
bool compareEventSkill(const Character &character, int rawId, int value, GameTitle title);
void setEventSkill(Character &character, int rawId, int value, GameTitle title);

}  // namespace mm6
