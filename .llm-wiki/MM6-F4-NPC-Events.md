---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, npc, campaign]
---

# MM6 F4 NPC Events

The [public packet](../project/MM6_F4_NPC_EVENTS.md) records 132 active topic writes and 29 NPC moves from F3-matched retained EVT data. A title-specific helper now bounds NPC, topic-slot, event and house IDs before mutating one `NPCData` record. The current `.ec6` RESC section copy-stages those fields; live saving remains guarded. Three focused native tests and the arm64 MM6/MM7/unit build passed.

The dialogue selector now sends an MM6 topic to a matching global EVT function before inherited MM7 teacher/guild/arena numeric ranges. The frozen campaign report identifies 74 references across 43 script IDs that previously met the MM7 teacher range. The 35 references without a matching global function or known special route remain guarded and need explicit MM6 classification. The aggregate checker rehashes all 68 retained active EVT resources and runs in the offline suite; exact sites stay in the private companion. F5 must inspect the conversation and changed NPC location, then save/quit/relaunch/load.
