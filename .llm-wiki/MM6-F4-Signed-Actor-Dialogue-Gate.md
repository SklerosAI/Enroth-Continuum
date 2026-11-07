---
type: analysis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Source-Sweep]]", "[[MM6-F4-V6-Indoor-Input-Cycle]]", "[[Campaign-Status]]"]
tags: [mm6, f4, indoor, npc, dialogue, headless]
---

# MM6 F4 Signed Actor Dialogue Gate

A pre-event fixture found an unresolved signed follower lookup; the original MM6 map-load trace now shows this source marker is cleared before gameplay.

The [project packet](../project/MM6_F4_SIGNED_ACTOR_DIALOGUE_GATE.md) retains the diagnostic provenance: before the map-load NPC step, the signed actor passed `CanInteractWithActor()` but `getNPCData()` rejected the value as an out-of-range follower position. The deliberate safe-address assertion failed **0/1** and the read-only characterization passed **1/1**. This pre-map observation is valid; the earlier claim of a persistent gameplay dialogue blocker is **superseded**.

A bounded [original-MM6 map-load trace](MM6-F4-Native-Signed-NPC-Map-Load.md) proves the ineligible source marker is cleared at runtime. The risk v6 writer checks post-world zero, and all three test-only five-child chains pass **15/15** with an MM7 control **1/1**. No real click or hire/fire behavior was tested. Eligible StreetNPC generation and positive actor dialogue lookup remain refused or unproved, so general indoor play and production SaveLoad stay closed.

See also: [Native Signed NPC Map Load](MM6-F4-Native-Signed-NPC-Map-Load.md), [Indoor Source Sweep](MM6-F4-Indoor-Source-Sweep.md), [V6 Indoor Input Cycle](MM6-F4-V6-Indoor-Input-Cycle.md), [Agent Handoff](Agent-Handoff.md), and [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
