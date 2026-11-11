#include "Engine/Evt/MM6ReputationEvent.h"

#include <stdexcept>

namespace mm6 {

int effectiveReputation(int reputation, bool (*isHired)(NpcProfession)) {
    int result = reputation;
    if (isHired(NPC_PROFESSION_BARD))
        result += 200;
    for (NpcProfession profession : {NPC_PROFESSION_PIRATE, NPC_PROFESSION_GYPSY, NPC_PROFESSION_DUPER, NPC_PROFESSION_BURGLAR})
        if (isHired(profession))
            result -= 200;
    return result;
}

bool compareReputation(int effectiveReputation, int value) {
    return effectiveReputation == value || value < 0;
}

int reputationAfterEventWrite(EvtOpcode opcode, int reputation, int value) {
    const int operand = value & 0xFF; // MM6 keeps only the low byte, so the shipped Subtract 1000 takes away 232.
    if (opcode == EVENT_Set)
        return operand;
    if (opcode == EVENT_Add)
        return reputation + operand;
    if (opcode == EVENT_Subtract)
        return reputation - operand;
    throw std::runtime_error("MM6 reputation event write has an invalid opcode");
}

} // namespace mm6
