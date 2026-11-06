#pragma once

#include "Engine/GameTitle.h"
#include "Engine/Tables/NPCTable.h"

namespace mm6 {

enum class NpcTopicRoute {
    Legacy,
    GlobalEvent,
    Guarded,
};

// MM6 global EVT functions take precedence over MM7's teacher/guild numeric
// ranges. A missing function is not evidence for any MM7 special topic.
NpcTopicRoute npcTopicRoute(GameTitle title, int eventId, bool hasGlobalEvent);

// Shipped MM6 SetNPCTopic and MoveNPC commands update table NPC state. The
// caller handles dialogue routing separately after these bounded mutations.
void setNpcTopic(NPCStats &stats, GameTitle title, int npcId, int slot, int eventId);
void moveNpc(NPCStats &stats, GameTitle title, int npcId, HouseId houseId);

} // namespace mm6
