#include "Engine/Evt/MM6NpcEventState.h"

#include <stdexcept>
#include <utility>

namespace mm6 {
namespace {

NPCData &checkedNpc(NPCStats &stats, GameTitle title, int npcId) {
    if (title != GameTitle::MM6 || npcId <= 0 ||
        static_cast<size_t>(npcId) >= stats.pNPCData.size())
        throw std::runtime_error("MM6 NPC event has invalid title or table NPC ID");
    return stats.pNPCData[npcId];
}

} // namespace

NpcTopicRoute npcTopicRoute(GameTitle title, int eventId, bool hasGlobalEvent) {
    if (title != GameTitle::MM6)
        return NpcTopicRoute::Legacy;
    return eventId > 0 && hasGlobalEvent ? NpcTopicRoute::GlobalEvent : NpcTopicRoute::Guarded;
}

void setNpcTopic(NPCStats &stats, GameTitle title, int npcId, int slot, int eventId) {
    // All retained MM6 writes use the first three script topic slots. Their
    // event IDs include zero, which removes a topic without a replacement.
    if (slot < 0 || slot > 2 || eventId < 0 || eventId > 364)
        throw std::runtime_error("MM6 NPC topic slot or event ID is outside retained range");
    NPCData &npc = checkedNpc(stats, title, npcId);
    switch (slot) {
        case 0: npc.dialogue_1_evt_id = eventId; break;
        case 1: npc.dialogue_2_evt_id = eventId; break;
        case 2: npc.dialogue_3_evt_id = eventId; break;
    }
}

void moveNpc(NPCStats &stats, GameTitle title, int npcId, HouseId houseId) {
    const int rawHouseId = std::to_underlying(houseId);
    // The retained MM6 house table has IDs 1..557. Zero removes an NPC from a
    // location; it occurs in shipped MoveNPC instructions.
    if (rawHouseId < 0 || rawHouseId > 557)
        throw std::runtime_error("MM6 NPC destination is outside retained house table");
    checkedNpc(stats, title, npcId).house = houseId;
}

} // namespace mm6
