#pragma once

#include "Engine/Evt/EvtEnums.h"
#include "Engine/Objects/NPCEnums.h"

namespace mm6 {

/**
 * @param reputation                    Stored party reputation, positive is good.
 * @param isHired                       Whether a hired NPC has the given profession.
 * @return                              Reputation that MM6 EVT variable 215 compares against. A hired Bard adds 200, and
 *                                      each of a hired Pirate, Gypsy, Duper and Burglar takes 200 away.
 */
int effectiveReputation(int reputation, bool (*isHired)(NpcProfession));

/**
 * @param effectiveReputation           Result of `effectiveReputation`.
 * @param value                         Compare operand.
 * @return                              Whether the compare passes. It's an equality test, and a negative operand
 *                                      passes whatever the reputation is.
 */
bool compareReputation(int effectiveReputation, int value);

/**
 * @param opcode                        `EVENT_Set`, `EVENT_Add` or `EVENT_Subtract`.
 * @param reputation                    Stored party reputation.
 * @param value                         Operand.
 * @return                              New stored reputation. Hired NPCs don't take part, and the result isn't clamped.
 */
int reputationAfterEventWrite(EvtOpcode opcode, int reputation, int value);

} // namespace mm6
