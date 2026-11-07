#include "Engine/Evt/MM6HirelingEvent.h"

#include <stdexcept>
#include <utility>

#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

namespace mm6 {

static void checkEventNpcId(const NPCStats &stats, int npcId) {
    if (npcId <= 0 || static_cast<size_t>(npcId) >= stats.pNPCData.size())
        throw std::runtime_error("MM6 hired NPC event operand is outside the NPC table");
}

bool isEventNpcHired(const NPCStats &stats, int npcId) {
    checkEventNpcId(stats, npcId);
    return static_cast<bool>(stats.pNPCData[npcId].flags & NPC_HIRED);
}

void setEventNpcHired(NPCStats &stats, int npcId, bool hired) {
    checkEventNpcId(stats, npcId);
    NPCData &npc = stats.pNPCData[npcId];
    if (hired)
        npc.flags |= NPC_HIRED;
    else
        npc.flags &= ~NPC_HIRED;
}

void dismissEventProfession(NPCStats &stats, Party &party, int profession) {
    if (!isValidEventProfession(profession))
        throw std::runtime_error("MM6 hired profession event operand is outside the profession table");
    for (int i = 0; i < stats.uNumNewNPCs; i++)
        if (stats.pNPCData[i].profession == static_cast<NpcProfession>(profession))
            stats.pNPCData[i].flags &= ~NPC_HIRED;
    for (NPCData &hireling : party.pHirelings)
        if (hireling.profession == static_cast<NpcProfession>(profession))
            hireling = NPCData();
}

bool isValidEventProfession(int profession) {
    return profession >= std::to_underlying(NPC_PROFESSION_FIRST) && profession <= std::to_underlying(NPC_PROFESSION_MM6_LAST);
}

} // namespace mm6
