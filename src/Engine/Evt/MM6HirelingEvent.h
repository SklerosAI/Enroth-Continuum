#pragma once

struct NPCStats;
struct Party;

namespace mm6 {

/**
 * @param stats                         NPC table.
 * @param npcId                         Operand of MM6 EVT variable 214, an NPC table index.
 * @return                              Whether that NPC travels with the party.
 */
bool isEventNpcHired(const NPCStats &stats, int npcId);

/**
 * Sets or clears the hired flag of an NPC, as MM6 EVT variable 214 does. Set and Add hire, Subtract dismisses. The
 * caller recounts the party's followers afterwards.
 *
 * @param stats                         NPC table.
 * @param npcId                         NPC table index.
 * @param hired                         New hired state.
 */
void setEventNpcHired(NPCStats &stats, int npcId, bool hired);

/**
 * Dismisses every hired table NPC with the given profession and clears the hireling slots that have it, as Subtract
 * on MM6 EVT variable 223 does. The caller recounts the party's followers afterwards.
 *
 * @param stats                         NPC table.
 * @param party                         Party whose hireling slots are checked.
 * @param profession                    Raw MM6 profession ID.
 */
void dismissEventProfession(NPCStats &stats, Party &party, int profession);

/**
 * @param profession                    Operand of MM6 EVT variable 223.
 * @return                              Whether it's a profession from the MM6 profession table.
 */
bool isValidEventProfession(int profession);

} // namespace mm6
