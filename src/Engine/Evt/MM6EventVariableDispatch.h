#pragma once

#include "Engine/Evt/EvtEnums.h"

class Character;

namespace mm6 {

/**
 * @param rawId                         MM6 EVT variable ID.
 * @return                              Whether it's an MM6 variable kept on the party or the NPC table and run by
 *                                      `comparePartyEventVariable` and `writePartyEventVariable`: hired NPC (214),
 *                                      day counters (216-221), hired profession (223) or circus prizes (224).
 */
bool isPartyEventVariable(int rawId);

/**
 * @param rawId                         MM6 party variable ID.
 * @param value                         Compare operand.
 * @return                              Compare result for the live party and NPC table.
 */
bool comparePartyEventVariable(int rawId, int value);

/**
 * @param opcode                        `EVENT_Set`, `EVENT_Add` or `EVENT_Subtract`.
 * @param rawId                         MM6 party variable ID.
 * @param value                         Operand.
 */
void writePartyEventVariable(EvtOpcode opcode, int rawId, int value);

/**
 * @param opcode                        `EVENT_Compare`, `EVENT_Set`, `EVENT_Add` or `EVENT_Subtract`.
 * @param rawId                         MM6 EVT variable ID.
 * @return                              Whether it's an MM6 per-character variable run by
 *                                      `compareCharacterEventVariable` and `writeCharacterEventVariable` for this
 *                                      opcode: resistances (46-55), conditions (87-104), skill points (225), awards
 *                                      (12) on Set and Add, and reputation (215). Reputation is shared by the party,
 *                                      but MM6 still runs it once per targeted character, so a write for the whole
 *                                      party lands four times.
 */
bool isCharacterEventVariable(EvtOpcode opcode, int rawId);

/**
 * @param character                     Selected character.
 * @param rawId                         MM6 per-character variable ID.
 * @param value                         Compare operand.
 * @return                              Compare result.
 */
bool compareCharacterEventVariable(const Character &character, int rawId, int value);

/**
 * @param character                     Selected character.
 * @param opcode                        `EVENT_Set`, `EVENT_Add` or `EVENT_Subtract`.
 * @param rawId                         MM6 per-character variable ID.
 * @param value                         Operand.
 */
void writeCharacterEventVariable(Character &character, EvtOpcode opcode, int rawId, int value);

} // namespace mm6
